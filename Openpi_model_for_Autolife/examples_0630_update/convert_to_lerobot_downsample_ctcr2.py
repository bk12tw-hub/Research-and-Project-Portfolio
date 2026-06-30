"""
Convert your robot's JSONL + video recordings to LeRobot format for pi0 training.

Your data structure (per session):
  - dataset_session_<name>.jsonl   → per-frame robot state/action data
  - videos/head_left/<name>.mp4    → head left camera
  - videos/head_right/<name>.mp4   → head right camera  
  - videos/hand_left/<name>.mp4    → hand/wrist left camera
  - videos/hand_right/<name>.mp4   → hand/wrist right camera

Your robot has 23 joints total (position in degrees).
We use joints [4:11] = left arm (7 DOF) and joints [11:18] = right arm (7 DOF).
Adjust ARM_JOINT_INDICES below to match your robot's actual joint mapping.

Usage:
    pip install lerobot opencv-python numpy tqdm
    python convert_to_lerobot.py \
        --data_dir /path/to/your/sessions \
        --repo_id your_hf_username/your_dataset_name \
        --task "pick up the red block" \
        --push_to_hub

Data directory structure expected:
    data_dir/
    ├── session_001/
    │   ├── joint
    │   │   └── joint_state.jsonl
    │   └── videos/
    │       ├── head_left/20260313_174013.mp4
    │       ├── head_right/20260313_174013.mp4
    │       ├── hand_left/20260313_174013.mp4
    │       └── hand_right/20260313_174013.mp4
    ├── session_002/
    │   └── ...
"""

import argparse
import json
import math
import os
from pathlib import Path

import cv2
import numpy as np
from tqdm import tqdm
from openpi_client import image_tools
# ─────────────────────────────────────────────
# CONFIGURATION — adjust these to match your robot
# ─────────────────────────────────────────────

# Your robot has 23 joints. Choose which ones are the arm joints.
# Based on your data: joints 4-10 look like arm joints (~20 deg range = ~0.35 rad)
# Joints 0-3 and 18-22 look like base/gripper joints (small values or fixed)
# CHANGE THESE to match your actual robot's joint layout:

# LEFT_ARM_JOINT_INDICES  = list(range(4, 11))   # 7 joints for left arm
# RIGHT_ARM_JOINT_INDICES = list(range(11, 18))  # 7 joints for right arm

#  changeed by HSU
LEFT_ARM_JOINT_INDICES  = list(range(4, 11))   # 7 arm joints
LEFT_GRIPPER_JOINT_INDEX  = 18                 # ← add your actual gripper index
RIGHT_ARM_JOINT_INDICES = list(range(11, 18))  # 7 arm joints  
RIGHT_GRIPPER_JOINT_INDEX = 19                 # ← add your actual gripper index

# Which camera to use as the main "image" and "wrist_image" for pi0
# Options: "head_left", "head_right", "hand_left", "hand_right"
MAIN_CAMERA   = "rgbd_head_color"    # → observation/image (base camera)
WRIST_CAMERA  = "hand_right"    # → observation/wrist_image

# Image resolution for saving (pi0 default is 224x224)
IMAGE_SIZE = (224, 224)

# Recording FPS (from your data: fps_target=30)
FPS = 6
DOWNSAMPLE_STRIDE = 5  # 30Hz / 2Hz = every 15th frame
# Action: use next-frame joint position as action (position control)
# Set to True to use joint velocity instead
USE_VELOCITY_AS_ACTION = False


DEBUG_VAL_DIR = Path("conversion_validation")
DEBUG_VAL_DIR.mkdir(exist_ok=True)

# ─────────────────────────────────────────────

def degrees_to_radians(deg_list):
    """Convert joint positions from degrees to radians."""
    return [math.radians(d) for d in deg_list]


def extract_state(frame: dict, use_left_arm: bool = True) -> list:
    """
    Extract robot state (joint positions in radians) from a JSONL frame.
    Returns a flat list of joint positions for one arm.
    """
    
    positions_deg = frame["joints"]["position"]
    indices = LEFT_ARM_JOINT_INDICES if use_left_arm else RIGHT_ARM_JOINT_INDICES
    gripper_idx = LEFT_GRIPPER_JOINT_INDEX if use_left_arm else RIGHT_GRIPPER_JOINT_INDEX
    
    arm_joints_rad    = degrees_to_radians([positions_deg[i] for i in indices])   
    left_gripper_norm = np.clip(positions_deg[gripper_idx] / 360.0, 0.0, 1.0)

    return arm_joints_rad + [float(left_gripper_norm)]

    # changed by HSU
    # positions_deg = frame["joints"]["position"]
    # indices  = LEFT_ARM_JOINT_INDICES if use_left_arm else RIGHT_ARM_JOINT_INDICES
    # gripper_idx = LEFT_GRIPPER_JOINT_INDEX if use_left_arm else RIGHT_GRIPPER_JOINT_INDEX
    # arm_joints = degrees_to_radians([positions_deg[i] for i in indices])
    # gripper    = [positions_deg[gripper_idx]]  # keep in degrees or normalize 0-1
    # return degrees_to_radians(selected) + gripper                # (8,)


def extract_action(current_frame: dict, next_frame: dict, use_left_arm: bool = True) -> list:
    """
    Extract action from frames.
    Action = next frame's joint position (position control).
    If USE_VELOCITY_AS_ACTION=True, uses current frame's joint velocity instead.
    """
    # indices = LEFT_ARM_JOINT_INDICES if use_left_arm else RIGHT_ARM_JOINT_INDICES

    # if USE_VELOCITY_AS_ACTION:
    #     # Use current frame velocity (already small values, keep as-is or convert)
    #     velocities = current_frame["joints"]["velocity"]
    #     selected = [velocities[i] for i in indices]
    #     return selected
    # else:
    #     # Use next frame's joint position as the target (position control)
    #     next_positions_deg = next_frame["joints"]["position"]
    #     selected = [next_positions_deg[i] for i in indices]
    #     return degrees_to_radians(selected)

    # changed by HSU
    indices     = LEFT_ARM_JOINT_INDICES if use_left_arm else RIGHT_ARM_JOINT_INDICES
    gripper_idx = LEFT_GRIPPER_JOINT_INDEX if use_left_arm else RIGHT_GRIPPER_JOINT_INDEX

    if USE_VELOCITY_AS_ACTION:
        velocities  = current_frame["joints"]["velocity"]
        arm_action  = [velocities[i] for i in indices]
        gripper_vel = [velocities[gripper_idx]]
        return arm_action + gripper_vel        # (8,)
    else:
        next_pos    = next_frame["joints"]["position"]
        arm_action  = degrees_to_radians([next_pos[i] for i in indices])
        gripper_act = np.clip(next_pos[gripper_idx] / 360.0, 0.0, 1.0)
        return arm_action + [float(gripper_act)]        # (8,)


def get_frame_from_video(cap: cv2.VideoCapture, frame_idx: int) -> np.ndarray:
    """Extract a specific frame from a video capture object."""
    cap.set(cv2.CAP_PROP_POS_FRAMES, frame_idx)
    ret, frame = cap.read()
    if not ret:
        return np.zeros((*IMAGE_SIZE, 3), dtype=np.uint8)
    # Convert BGR → RGB and resize
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
   
    # frame = image_tools.resize_with_pad(frame, 256, 256)

    # 2. center crop → 224x224
    h, w = frame.shape[:2]
    # 1. 找出短邊，決定正方形大小
    min_edge = min(h, w)
    top = (h - min_edge) // 2
    left = (w - min_edge) // 2
    
    # 2. 進行裁剪
    cropped = frame[top:top+min_edge, left:left+min_edge]
    resized = cv2.resize(cropped, (224, 224), interpolation=cv2.INTER_AREA)

    return resized


def load_session(session_dir: Path):
    """
    Load all JSONL frames and video captures for one session.
    Returns (frames, video_caps) where video_caps is a dict of camera→VideoCapture.
    """
    # Find JSONL file
    jsonl_files = list((session_dir / "joints").glob("*.jsonl"))
    if not jsonl_files:
        raise FileNotFoundError(f"No JSONL file found in {session_dir}")
    jsonl_path = jsonl_files[0]

    # Load all frames
    frames = []
    with open(jsonl_path) as f:
        for line in f:
            line = line.strip()
            if line:
                frames.append(json.loads(line))

    if not frames:
        raise ValueError(f"No frames in {jsonl_path}")

    # Get session name from first frame
    session_name = frames[0]["session_name"]

    # Open video captures
    video_dir = session_dir / "videos"
    video_caps = {}
    for cam_name in [MAIN_CAMERA, WRIST_CAMERA]:
        video_path = video_dir / f"{cam_name}.mp4"
        if video_path.exists():
            cap = cv2.VideoCapture(str(video_path))
            video_caps[cam_name] = cap
        else:
            print(f"  Warning: video not found: {video_path}")
            video_caps[cam_name] = None

    return frames, video_caps


def convert_sessions_to_lerobot(
    data_dir: str,
    repo_id: str,
    task: str = "robot manipulation task",
    push_to_hub: bool = False,
    use_left_arm: bool = True,
):
    """
    Main conversion function. Iterates over all session directories and
    converts them into a single LeRobot dataset.
    """
    from lerobot.common.datasets.lerobot_dataset import LeRobotDataset

    data_path = Path(data_dir)
    arm_name = "left" if use_left_arm else "right"
    arm_indices = LEFT_ARM_JOINT_INDICES if use_left_arm else RIGHT_ARM_JOINT_INDICES
    action_dim = 8
    state_dim  = 8  

    print(f"Converting {arm_name} arm, joint indices: {arm_indices}")
    print(f"State dim: {state_dim}, Action dim: {action_dim}")
    print(f"Action type: {'velocity' if USE_VELOCITY_AS_ACTION else 'position (radians)'}")

    # Create LeRobot dataset
    # Note: openpi expects "action" (singular), NOT "actions"
    dataset = LeRobotDataset.create(
        repo_id=repo_id,
        robot_type="panda",
        fps=FPS,
        features={
            "image": {
                "dtype": "image",
                "shape": (*IMAGE_SIZE, 3),
                "names": ["height", "width", "channel"],
            },
            "wrist_image": {
                "dtype": "image",
                "shape": (*IMAGE_SIZE, 3),
                "names": ["height", "width", "channel"],
            },
            "state": {
                "dtype": "float32",
                "shape": (8,),
                "names": ["state"],
            },
            "actions": {
                "dtype": "float32",
                "shape": (8,),
                "names": ["actions"],
            },
        },
        image_writer_threads=10,
        image_writer_processes=5,
    )

    # Find all session directories
    session_dirs = sorted([
        d for d in data_path.iterdir()
        if d.is_dir() and any(d.glob("joints/*.jsonl")) 
    ])

    # Also handle flat structure (JSONL directly in data_dir)
    if not session_dirs and list(data_path.glob("*.jsonl")):
        session_dirs = [data_path]

    if not session_dirs:
        raise ValueError(f"No session directories with JSONL files found in {data_dir}")

    print(f"\nFound {len(session_dirs)} session(s)")
    total_frames = 0

    for session_dir in tqdm(session_dirs, desc="Sessions"):
        print(f"\nProcessing: {session_dir.name}")

        try:
            frames, video_caps = load_session(session_dir)
        except Exception as e:
            print(f"  Skipping {session_dir.name}: {e}")
            continue

        n_frames = len(frames)
        print(f"  Frames: {n_frames}")

        # We need at least 2 frames to compute next-frame actions
        if n_frames < 2:
            print(f"  Skipping: too few frames")
            continue


        # ──────────────────────────────────────────────────────────
        # VALIDATION BLOCK: Check first frame of each session
        # ──────────────────────────────────────────────────────────
        test_i = 0
        state_test = extract_state(frames[test_i], use_left_arm)
        
        print(f"\n--- Validation [Session: {session_dir.name}] ---")
        print(f"Joints (Rad): {np.round(state_test[:7], 4)}")
        print(f"Gripper (0-1): {state_test[7]:.4f}")
        
        # Check if Radians make sense (typically between -3.14 and 3.14)
        if any(abs(j) > 6.28 for j in state_test[:7]):
            print("⚠️ WARNING: Radians look suspiciously large. Are indices correct?")
        
        # Add this before line 305
        print(f"DEBUG: WRIST_CAMERA key is: {WRIST_CAMERA}")
        print(f"DEBUG: video_caps keys are: {list(video_caps.keys())}")
        wrist_cap = video_caps.get(WRIST_CAMERA)
        print(f"DEBUG: wrist_cap object: {wrist_cap}")

        if wrist_cap is None:
            raise FileNotFoundError(f"Could not find or open the video for {WRIST_CAMERA}. Check your file paths!")

        # Save images to disk for visual inspection
        img_check = get_frame_from_video(video_caps.get(MAIN_CAMERA), test_i)
        wrist_check = get_frame_from_video(video_caps.get(WRIST_CAMERA), test_i)
        
        # Save as BGR for OpenCV imwrite
        combined_check = np.hstack([cv2.cvtColor(img_check, cv2.COLOR_RGB2BGR), 
                                cv2.cvtColor(wrist_check, cv2.COLOR_RGB2BGR)])
        
        check_filename = DEBUG_VAL_DIR / f"val_{session_dir.name}.jpg"
        # cv2.imwrite(str(check_filename), combined_check)
        print(f"Validation image saved to: {check_filename}")
        print(f"Image Resolution: {img_check.shape[1]}x{img_check.shape[0]}")

        # ──────────────────────────────────────────────────────────

        # Extract and add each frame (skip last frame — no next action)
        # for i in range(n_frames - 1):
        #     frame      = frames[i]
        #     next_frame = frames[i + 1]

        #     # State: current joint positions
        #     state = extract_state(frame, use_left_arm)

        #     # Action: next joint positions (or current velocity)
        #     action = extract_action(frame, next_frame, use_left_arm)

        #     # Images from video
        #     main_img  = get_frame_from_video(video_caps.get(MAIN_CAMERA),  i) #\
        #                 # if video_caps.get(MAIN_CAMERA) else \
        #                 # np.zeros((*IMAGE_SIZE, 3), dtype=np.uint8)

        #     wrist_img = get_frame_from_video(video_caps.get(WRIST_CAMERA), i) #\
        #                 # if video_caps.get(WRIST_CAMERA) else \
        #                 # np.zeros((*IMAGE_SIZE, 3), dtype=np.uint8)

        #     dataset.add_frame({
        #         "image":        main_img,
        #         "wrist_image":  wrist_img,
        #         "state":        np.array(state,  dtype=np.float32),
        #         "actions":      np.array(action, dtype=np.float32),
        #         "task":         task,
        #     })
        # Build list of sampled indices first
        sampled_indices = list(range(0, n_frames - DOWNSAMPLE_STRIDE, DOWNSAMPLE_STRIDE))

        for idx, i in enumerate(sampled_indices):
            frame      = frames[i]
            next_frame = frames[i + DOWNSAMPLE_STRIDE]  # "next" in 2Hz space

            state  = extract_state(frame, use_left_arm)
            action = extract_action(frame, next_frame, use_left_arm)

            main_img  = get_frame_from_video(video_caps.get(MAIN_CAMERA),  i)
            wrist_img = get_frame_from_video(video_caps.get(WRIST_CAMERA), i)

            dataset.add_frame({
                "image":       main_img,
                "wrist_image": wrist_img,
                "state":       np.array(state,  dtype=np.float32),
                "actions":     np.array(action, dtype=np.float32),
                "task":        task,
            })

        total_frames += len(sampled_indices)
        print(f"  Frames: {n_frames} raw → {len(sampled_indices)} sampled at {FPS}Hz")

        # Save this episode
        dataset.save_episode()
        total_frames += n_frames - 1

        # Release video captures
        for cap in video_caps.values():
            if cap is not None:
                cap.release()

    print(f"\nConversion complete!")
    print(f"Total frames: {total_frames}")
    print(f"Dataset saved locally to: {dataset.root}")

    # Push to HF Hub
    if push_to_hub:
        print(f"\nPushing to Hugging Face Hub as '{repo_id}'...")
        dataset.push_to_hub(
            tags=["robot", "manipulation", "pi0"],
            private=True,
            push_videos=True,
            license="apache-2.0",
        )
        print(f"Done! View at: https://huggingface.co/datasets/{repo_id}")

    return dataset


def main():
    parser = argparse.ArgumentParser(description="Convert robot JSONL+video to LeRobot format")
    parser.add_argument("--data_dir",    required=True,  help="Directory containing session folders")
    parser.add_argument("--repo_id",     required=True,  help="HF repo ID, e.g. your_username/my_dataset")
    parser.add_argument("--task",        default="robot manipulation task", help="Task description / language instruction")
    parser.add_argument("--arm",         default="left", choices=["left", "right"], help="Which arm to use")
    parser.add_argument("--push_to_hub", action="store_true", help="Push dataset to Hugging Face Hub")
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