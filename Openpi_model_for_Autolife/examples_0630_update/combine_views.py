"""
combine_views.py

Layout:
  [ cam1 (224x224) ]  |
  [ cam2 (224x224) ]  |  [ third_view (rotated 90°, downsampled to 6hz) ]

The two cam images are stacked vertically → 224x448
The third-view is rotated 90° then resized to match height 448.
Both halves are placed side by side → final video.

Usage:
    python combine_views.py \
        --frames path/to/frames_folder/ \
        --third  path/to/third_view.mp4 \
        --output combined.mp4 \
        --fps    6

    Expects filenames like: 0001_main.jpg, 0001_wrist.jpg, 0002_main.jpg ...
"""

import cv2
import argparse
import os
import glob
import sys


def load_frames_by_suffix(folder: str, suffix: str) -> list:
    """Load frames matching *_suffix.jpg/png from a folder, sorted by filename."""
    exts = ("jpg", "jpeg", "png")
    files = []
    for ext in exts:
        files.extend(glob.glob(os.path.join(folder, f"*_{suffix}.{ext}")))
    if not files:
        print(f"[ERROR] No *_{suffix}.jpg/png found in {folder}")
        sys.exit(1)
    files.sort()
    print(f"[INFO] Found {len(files)} '{suffix}' frames in {folder}")
    return files


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--frames", required=True, help="Folder containing *_main and *_wrist frames")
    parser.add_argument("--third",  required=True, help="Third-view video file (.mp4 etc)")
    parser.add_argument("--output", default="combined.mp4", help="Output video path")
    parser.add_argument("--fps",    type=float, default=6.0, help="Output FPS (default: 6)")
    args = parser.parse_args()

    # ── Load cam frames ──────────────────────────────────────────────
    cam1_files = load_frames_by_suffix(args.frames, "main")
    cam2_files = load_frames_by_suffix(args.frames, "wrist")

    n_cam_frames = min(len(cam1_files), len(cam2_files))
    print(f"[INFO] Using {n_cam_frames} cam frames (shortest of main/wrist)")

    # ── Open third-view video ────────────────────────────────────────
    cap = cv2.VideoCapture(args.third)
    if not cap.isOpened():
        print(f"[ERROR] Cannot open {args.third}")
        sys.exit(1)

    third_total   = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    third_src_fps = cap.get(cv2.CAP_PROP_FPS) or 30.0
    TARGET_FPS    = 3.5
    # Step between source frames to achieve ~6hz
    third_step    = third_src_fps / TARGET_FPS
    third_downsampled = int(third_total / third_step)
    print(f"[INFO] Third-view: {third_total} frames @ {third_src_fps:.1f}fps")
    print(f"[INFO] Downsampling to {TARGET_FPS}hz → ~{third_downsampled} frames (step={third_step:.2f})")

    n_frames = min(n_cam_frames, third_downsampled)
    print(f"[INFO] Final frame count: {n_frames}")

    # ── Compute output dimensions ────────────────────────────────────
    cam_w, cam_h = 224, 224
    stack_h = cam_h * 2   # 448
    stack_w = cam_w       # 224

    # Third-view: after 90° rotation width and height swap
    # orig_w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    # orig_h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    rotated_w = 1080   # e.g. 1080
    rotated_h = 1920   # e.g. 1920  (tall after rotation)
    scale   = stack_h / rotated_h
    third_w = int(rotated_w * scale)
    third_h = stack_h    # 448

    out_w = stack_w + third_w
    out_h = stack_h
    print(f"[INFO] Output size: {out_w}x{out_h}")
    print(f"       cam stack: {stack_w}x{stack_h} | third (post-rotate+resize): {third_w}x{third_h}")

    # ── VideoWriter ──────────────────────────────────────────────────
    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    writer = cv2.VideoWriter(args.output, fourcc, args.fps, (out_w, out_h))

    # ── Main loop ────────────────────────────────────────────────────
    third_frame_idx = 0.0   # floating point position in source video

    for i in range(n_frames):
        # Read cam1
        f1 = cv2.imread(cam1_files[i])
        if f1 is None:
            print(f"[WARN] Cannot read {cam1_files[i]}, skipping")
            continue
        f1 = cv2.resize(f1, (cam_w, cam_h))

        # Read cam2
        f2 = cv2.imread(cam2_files[i])
        if f2 is None:
            print(f"[WARN] Cannot read {cam2_files[i]}, skipping")
            continue
        f2 = cv2.resize(f2, (cam_w, cam_h))

        # Seek to the correct source frame for this output frame
        target_src_frame = int(third_frame_idx)
        cap.set(cv2.CAP_PROP_POS_FRAMES, target_src_frame)
        ret, f3 = cap.read()
        if not ret:
            print(f"[WARN] Third-view ended at output frame {i}")
            break

        # Rotate 90° clockwise
        # f3 = cv2.rotate(f3, cv2.ROTATE_90_CLOCKWISE)

        # Resize to match stack height
        f3 = cv2.resize(f3, (third_w, third_h))

        third_frame_idx += third_step

        # Stack cam1 + cam2 vertically
        cam_stack = cv2.vconcat([f1, f2])   # 224 x 448

        # Concatenate horizontally with third-view
        combined = cv2.hconcat([cam_stack, f3])

        writer.write(combined)

        if (i + 1) % 50 == 0:
            print(f"[INFO] Processed {i+1}/{n_frames} frames")

    cap.release()
    writer.release()
    print(f"[DONE] Saved to {args.output}")


if __name__ == "__main__":
    main()




# import cv2
# import argparse
# import os
# import glob
# import sys


# def load_frames_by_suffix(folder: str, suffix: str) -> list:
#     """Load frames matching *_suffix.jpg/png from a folder, sorted by filename."""
#     exts = ("jpg", "jpeg", "png")
#     files = []
#     for ext in exts:
#         files.extend(glob.glob(os.path.join(folder, f"*_{suffix}.{ext}")))
#     if not files:
#         print(f"[ERROR] No *_{suffix}.jpg/png found in {folder}")
#         sys.exit(1)
#     files.sort()
#     print(f"[INFO] Found {len(files)} '{suffix}' frames in {folder}")
#     return files


# def main():
#     parser = argparse.ArgumentParser()
#     parser.add_argument("--frames", required=True, help="Folder containing *_main and *_wrist frames")
#     parser.add_argument("--third",  required=True, help="Third-view video file (.mp4 etc)")
#     parser.add_argument("--output", default="combined.mp4", help="Output video path")
#     parser.add_argument("--fps",    type=float, default=6.0, help="Output FPS (default: 6)")
#     args = parser.parse_args()

#     # ── Load cam frames ──────────────────────────────────────────────
#     cam1_files = load_frames_by_suffix(args.frames, "main")
#     cam2_files = load_frames_by_suffix(args.frames, "wrist")

#     n_cam_frames = min(len(cam1_files), len(cam2_files))
#     print(f"[INFO] Using {n_cam_frames} cam frames (shortest of main/wrist)")

#     # ── Open third-view video ────────────────────────────────────────
#     cap = cv2.VideoCapture(args.third)
#     if not cap.isOpened():
#         print(f"[ERROR] Cannot open {args.third}")
#         sys.exit(1)

#     third_total   = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
#     third_src_fps = cap.get(cv2.CAP_PROP_FPS) or 30.0
#     TARGET_FPS    = 2.9
    
#     third_step    = third_src_fps / TARGET_FPS
#     third_downsampled = int(third_total / third_step)
#     print(f"[INFO] Third-view: {third_total} frames @ {third_src_fps:.1f}fps")
#     print(f"[INFO] Downsampling to {TARGET_FPS}hz → ~{third_downsampled} frames (step={third_step:.2f})")

#     n_frames = min(n_cam_frames, third_downsampled)
#     print(f"[INFO] Final frame count: {n_frames}")

#     # ── Force Dimensions to hit exactly 476x448 ──────────────────────
#     cam_w, cam_h = 224, 224
#     stack_h = cam_h * 2   # 448
#     stack_w = cam_w       # 224

#     # To get 476 total width, third_w MUST be exactly 252 (476 - 224)
#     third_w = 252
#     third_h = stack_h     # 448

#     out_w = 476
#     out_h = 448
#     print(f"[INFO] Enforcing target output size: {out_w}x{out_h}")
#     print(f"       cam stack: {stack_w}x{stack_h} | third (cropped+resized): {third_w}x{third_h}")

#     # ── VideoWriter ──────────────────────────────────────────────────
#     fourcc = cv2.VideoWriter_fourcc(*"mp4v")
#     writer = cv2.VideoWriter(args.output, fourcc, args.fps, (out_w, out_h))

#     # ── Main loop ────────────────────────────────────────────────────
#     current_video_frame = 0

#     for i in range(n_frames):
#         # Read cam1 & cam2
#         f1 = cv2.imread(cam1_files[i])
#         f2 = cv2.imread(cam2_files[i])
#         if f1 is None or f2 is None:
#             print(f"[WARN] Cannot read frames at index {i}, skipping")
#             continue
            
#         f1 = cv2.resize(f1, (cam_w, cam_h))
#         f2 = cv2.resize(f2, (cam_w, cam_h))

#         # Calculate target frame index in the source video
#         target_src_frame = int(i * third_step)
        
#         # Fast-forward decode loop (much quicker than cap.set)
#         while current_video_frame < target_src_frame:
#             cap.grab()
#             current_video_frame += 1

#         ret, f3 = cap.read()
#         current_video_frame += 1
#         if not ret:
#             print(f"[WARN] Third-view ended early at frame {i}")
#             break

#         # 1. Rotate 90° clockwise
#         # f3 = cv2.rotate(f3, cv2.ROTATE_90_CLOCKWISE)

#         # 2. Center-crop the frame to match the target 252:448 aspect ratio
#         h_rot, w_rot = f3.shape[:2]
#         target_aspect = third_w / third_h  # 252 / 448 ≈ 0.5625
#         current_aspect = w_rot / h_rot

#         if current_aspect > target_aspect:
#             # Frame is too wide, crop the sides
#             new_w = int(h_rot * target_aspect)
#             start_x = (w_rot - new_w) // 2
#             f3 = f3[:, start_x:start_x + new_w]
#         else:
#             # Frame is too tall, crop top/bottom
#             new_h = int(w_rot / target_aspect)
#             start_y = (h_rot - new_h) // 2
#             f3 = f3[start_y:start_y + new_h, :]

#         # 3. Resize safely to exact dimensions
#         f3 = cv2.resize(f3, (third_w, third_h))

#         # Stack layouts
#         cam_stack = cv2.vconcat([f1, f2])   
#         combined = cv2.hconcat([cam_stack, f3])

#         writer.write(combined)

#         if (i + 1) % 50 == 0:
#             print(f"[INFO] Processed {i+1}/{n_frames} frames")

#     cap.release()
#     writer.release()
#     print(f"[DONE] Saved to {args.output}")


# if __name__ == "__main__":
#     main()