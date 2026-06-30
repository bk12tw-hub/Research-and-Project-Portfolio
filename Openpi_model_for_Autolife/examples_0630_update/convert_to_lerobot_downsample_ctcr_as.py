"""
Convert your robot's JSONL + video recordings to LeRobot format for pi0 training.

Improvements over original:
  1. Fixed total_frames double-count bug
  2. Action smoothing (Savitzky-Golay) on joint sequences before conversion
  3. Last-N-frames reweighting via episode duplication
  4. Per-session validation report
  5. Cleaner downsampling with explicit boundary check

Usage:
    python convert_to_lerobot.py \
        --data_dir /path/to/your/sessions \
        --repo_id your_hf_username/your_dataset_name \
        --task "pick up toy" \
        --arm right \
        --push_to_hub
"""

import argparse
import json
import math
import os
from pathlib import Path

import cv2
import numpy as np
from scipy.signal import savgol_filter
from tqdm import tqdm
from openpi_client import image_tools

# ─────────────────────────────────────────────
# CONFIGURATION
# ─────────────────────────────────────────────
LEFT_ARM_JOINT_INDICES    = list(range(4, 11))
LEFT_GRIPPER_JOINT_INDEX  = 18
RIGHT_ARM_JOINT_INDICES   = list(range(11, 18))
RIGHT_GRIPPER_JOINT_INDEX = 19

MAIN_CAMERA  = "rgbd_head_color"
WRIST_CAMERA = "hand_right"
IMAGE_SIZE   = (224, 224)

FPS               = 2
DOWNSAMPLE_STRIDE = 15    # 30Hz raw → 2Hz dataset (every 15th frame)

USE_VELOCITY_AS_ACTION = False

# ── Action Smoothing ──────────────────────────────────────────────
# Applied on the RAW 30Hz joint sequence BEFORE downsampling.
# Savitzky-Golay: preserves peaks/edges better than simple moving average.
SMOOTH_ACTIONS       = True
SMOOTH_WINDOW        = 15    # must be odd; ~0.5s at 30Hz
SMOOTH_POLY          = 2     # polynomial order; 2 is a good default

# ── Last-N-Frames Reweighting ─────────────────────────────────────
# Duplicate the last portion of each episode so the model sees the
# "approach and grasp" phase more often during training.
# This compensates for the short duration of the fine-grained grasping phase.
REWEIGHT_LAST_FRAMES = True
REWEIGHT_FRACTION    = 0.5  # last 25% of each episode = "approach" phase
REWEIGHT_MULTIPLIER  = 2     # repeat that portion N times

# ─────────────────────────────────────────────

DEBUG_VAL_DIR = Path("conversion_validation")
DEBUG_VAL_DIR.mkdir(exist_ok=True)


def degrees_to_radians(deg_list):
    return [math.radians(d) for d in deg_list]


def smooth_joint_sequence(frames: list[dict], use_left_arm: bool) -> np.ndarray:
    """
    Extract the full joint sequence from all frames and apply
    Savitzky-Golay smoothing on each joint dimension independently.

    Why SavGol over simple moving average:
      - Preserves peaks and edges (important for grasp timing)
      - Removes high-frequency encoder noise without adding phase lag
      - Works well on short sequences

    Returns smoothed array of shape (n_frames, 8): [7 joints (rad), gripper (0-1)]
    """
    arm_indices = LEFT_ARM_JOINT_INDICES if use_left_arm else RIGHT_ARM_JOINT_INDICES
    gripper_idx = LEFT_GRIPPER_JOINT_INDEX if use_left_arm else RIGHT_GRIPPER_JOINT_INDEX

    raw = []
    for f in frames:
        pos = f["joints"]["position"]
        arm_rad  = [math.radians(pos[i]) for i in arm_indices]
        gripper  = float(np.clip(pos[gripper_idx] / 360.0, 0.0, 1.0))
        raw.append(arm_rad + [gripper])

    raw = np.array(raw, dtype=np.float32)   # (T, 8)

    if not SMOOTH_ACTIONS or len(raw) < SMOOTH_WINDOW:
        return raw

    smoothed = np.zeros_like(raw)
    for dim in range(raw.shape[1]):
        if dim == 7:
            # Don't smooth gripper — it's typically a discrete open/close signal.
            # Smoothing it creates a "half-open" state that causes weak grasps.
            smoothed[:, dim] = raw[:, dim]
        else:
            smoothed[:, dim] = savgol_filter(raw[:, dim], SMOOTH_WINDOW, SMOOTH_POLY)

    return smoothed   # (T, 8)


def get_frame_from_video(cap: cv2.VideoCapture, frame_idx: int) -> np.ndarray:
    """Extract a specific frame from video, resize to 224x224 via 256 center-crop."""
    if cap is None:
        return np.zeros((*IMAGE_SIZE, 3), dtype=np.uint8)
    cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
    ret, frame = cap.read()
    if not ret:
        return np.zeros((*IMAGE_SIZE, 3), dtype=np.uint8)

    frame     = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    frame_256 = image_tools.resize_with_pad(frame, 256, 256)

    start_x = (256 - 224) // 2
    start_y = (256 - 224) // 2
    return frame_256[start_y:start_y+224, start_x:start_x+224]


def load_session(session_dir: Path):
    """Load JSONL frames and video captures for one session."""
    jsonl_files = list((session_dir / "joints").glob("*.jsonl"))
    if not jsonl_files:
        raise FileNotFoundError(f"No JSONL file in {session_dir}")

    frames = []
    with open(jsonl_files[0]) as f:
        for line in f:
            line = line.strip()
            if line:
                frames.append(json.loads(line))

    if not frames:
        raise ValueError(f"No frames in {jsonl_files[0]}")

    video_dir  = session_dir / "videos"
    video_caps = {}
    for cam in [MAIN_CAMERA, WRIST_CAMERA]:
        path = video_dir / f"{cam}.mp4"
        video_caps[cam] = cv2.VideoCapture(str(path)) if path.exists() else None
        if video_caps[cam] is None:
            print(f"  ⚠️  Video not found: {path}")

    return frames, video_caps


def build_sampled_indices(n_frames: int) -> list[int]:
    """
    Build downsampled frame indices at 2Hz from a 30Hz recording.
    Ensures next_frame = i + DOWNSAMPLE_STRIDE is always valid.
    """
    indices = []
    i = 0
    while i + DOWNSAMPLE_STRIDE < n_frames:
        indices.append(i)
        i += DOWNSAMPLE_STRIDE
    return indices


def validate_session(session_dir, frames, video_caps, smoothed_seq, use_left_arm):
    """Print a per-session validation report and save a side-by-side image."""
    print(f"\n--- Validation [{session_dir.name}] ---")
    state = smoothed_seq[0]
    print(f"  Joints (rad) : {np.round(state[:7], 4)}")
    print(f"  Gripper (0-1): {state[7]:.4f}")

    if any(abs(j) > 6.28 for j in state[:7]):
        print("  ⚠️  WARNING: Radians look large — check joint indices!")

    # Action magnitude check: large actions = model has to predict big jumps
    if len(smoothed_seq) > DOWNSAMPLE_STRIDE:
        delta = smoothed_seq[DOWNSAMPLE_STRIDE] - smoothed_seq[0]
        print(f"  Action delta (step 0→1): {np.round(delta[:7], 4)}")
        if np.any(np.abs(delta[:7]) > 0.5):
            print("  ⚠️  WARNING: Large action delta (>0.5 rad). "
                  "Consider if DOWNSAMPLE_STRIDE is appropriate.")

    # Save side-by-side validation image
    img   = get_frame_from_video(video_caps.get(MAIN_CAMERA),  0)
    wrist = get_frame_from_video(video_caps.get(WRIST_CAMERA), 0)
    combined = np.hstack([
        cv2.cvtColor(img,   cv2.COLOR_RGB2BGR),
        cv2.cvtColor(wrist, cv2.COLOR_RGB2BGR),
    ])
    out = DEBUG_VAL_DIR / f"val_{session_dir.name}.jpg"
    cv2.imwrite(str(out), combined)
    print(f"  Validation image → {out}")


def add_episode_frames(dataset, sampled_indices, smoothed_seq, video_caps, task):
    """Add one episode's frames to the dataset."""
    for i in sampled_indices:
        state  = smoothed_seq[i].astype(np.float32)                 # current state
        action = smoothed_seq[i + DOWNSAMPLE_STRIDE].astype(np.float32)  # next state = target

        main_img  = get_frame_from_video(video_caps.get(MAIN_CAMERA),  i)
        wrist_img = get_frame_from_video(video_caps.get(WRIST_CAMERA), i)

        dataset.add_frame({
            "image":       main_img,
            "wrist_image": wrist_img,
            "state":       state,
            "actions":     action,
            "task":        task,
        })


def convert_sessions_to_lerobot(
    data_dir: str,
    repo_id: str,
    task: str = "pick up toy",
    push_to_hub: bool = False,
    use_left_arm: bool = True,
):
    from lerobot.common.datasets.lerobot_dataset import LeRobotDataset

    data_path = Path(data_dir)

    dataset = LeRobotDataset.create(
        repo_id=repo_id,
        robot_type="panda",
        fps=FPS,
        features={
            "image":       {"dtype": "image",   "shape": (*IMAGE_SIZE, 3), "names": ["height", "width", "channel"]},
            "wrist_image": {"dtype": "image",   "shape": (*IMAGE_SIZE, 3), "names": ["height", "width", "channel"]},
            "state":       {"dtype": "float32", "shape": (8,),             "names": ["state"]},
            "actions":     {"dtype": "float32", "shape": (8,),             "names": ["actions"]},
        },
        image_writer_threads=10,
        image_writer_processes=5,
    )

    session_dirs = sorted([
        d for d in data_path.iterdir()
        if d.is_dir() and any(d.glob("joints/*.jsonl"))
    ])
    if not session_dirs:
        raise ValueError(f"No session dirs found in {data_dir}")

    print(f"Found {len(session_dirs)} session(s)")
    print(f"Smoothing: {SMOOTH_ACTIONS} (window={SMOOTH_WINDOW}, poly={SMOOTH_POLY})")
    print(f"Reweighting last {REWEIGHT_FRACTION*100:.0f}% × {REWEIGHT_MULTIPLIER}: {REWEIGHT_LAST_FRAMES}")

    total_frames = 0

    for session_dir in tqdm(session_dirs, desc="Sessions"):
        print(f"\nProcessing: {session_dir.name}")

        try:
            frames, video_caps = load_session(session_dir)
        except Exception as e:
            print(f"  Skipping: {e}")
            continue

        n_frames = len(frames)
        if n_frames < DOWNSAMPLE_STRIDE + 1:
            print(f"  Skipping: too few frames ({n_frames})")
            continue

        # ── 1. Smooth the full 30Hz joint sequence ─────────────────
        smoothed_seq = smooth_joint_sequence(frames, use_left_arm)   # (T, 8)

        # ── 2. Validate ────────────────────────────────────────────
        validate_session(session_dir, frames, video_caps, smoothed_seq, use_left_arm)

        # ── 3. Build downsampled indices ───────────────────────────
        sampled = build_sampled_indices(n_frames)
        if not sampled:
            print(f"  Skipping: no valid indices after downsampling")
            continue
        print(f"  {n_frames} raw frames → {len(sampled)} sampled at {FPS}Hz")

        # ── 4. Add full episode ────────────────────────────────────
        add_episode_frames(dataset, sampled, smoothed_seq, video_caps, task)
        dataset.save_episode()
        total_frames += len(sampled)

        # ── 5. Reweight last N frames (approach + grasp phase) ─────
        # if REWEIGHT_LAST_FRAMES and len(sampled) > 4:
        #     n_last   = max(1, int(len(sampled) * REWEIGHT_FRACTION))
        #     last_idx = sampled[-n_last:]

        #     for repeat in range(REWEIGHT_MULTIPLIER - 1):
        #         add_episode_frames(dataset, last_idx, smoothed_seq, video_caps, task)
        #         dataset.save_episode()
        #         total_frames += len(last_idx)
        #         print(f"  Reweight repeat {repeat+1}: added {len(last_idx)} frames "
        #               f"(last {REWEIGHT_FRACTION*100:.0f}% of episode)")

        # ── 6. Release video captures ──────────────────────────────
        for cap in video_caps.values():
            if cap is not None:
                cap.release()

    print(f"\n✅ Conversion complete!")
    print(f"   Total frames : {total_frames}")
    print(f"   Dataset root : {dataset.root}")

    if push_to_hub:
        print(f"\nPushing to Hub as '{repo_id}'...")
        dataset.push_to_hub(
            tags=["robot", "manipulation", "pi0"],
            private=True,
            push_videos=True,
            license="apache-2.0",
        )
        print(f"Done! https://huggingface.co/datasets/{repo_id}")

    return dataset


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--data_dir",    required=True)
    parser.add_argument("--repo_id",     required=True)
    parser.add_argument("--task",        default="pick up toy")
    parser.add_argument("--arm",         default="right", choices=["left", "right"])
    parser.add_argument("--push_to_hub", action="store_true")
    args = parser.parse_args()

    convert_sessions_to_lerobot(
        data_dir=args.data_dir,
        repo_id=args.repo_id,
        task=args.task,
        push_to_hub=args.push_to_hub,
        use_left_arm=(args.arm == "left"),
    )


if __name__ == "__main__":
    main()