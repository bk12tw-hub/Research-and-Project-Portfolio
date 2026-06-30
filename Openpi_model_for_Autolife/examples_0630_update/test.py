# import pandas as pd
# import numpy as np
# import cv2
# from pathlib import Path

# # Update this path to your exact episode parquet file
# PARQUET_FILE = "/home/lenovo/.cache/huggingface/lerobot/Chi-chu/dataset_pick_cube/data/chunk-000/episode_000000.parquet"

# def inspect_clean_format():
#     parquet_path = Path(PARQUET_FILE)
#     if not parquet_path.exists():
#         print(f"Error: Parquet file not found at {PARQUET_FILE}")
#         return

#     # Load the single episode parquet file
#     df = pd.read_parquet(parquet_path)
    
#     print("\n" + "="*60)
#     print(f"📊 DATASET SHARD HEALTH REPORT")
#     print("="*60)
#     print(f"File Name:              {parquet_path.name}")
#     print(f"Total Logged Frames:    {len(df)}")
#     print("-"*60)

#     # Inspect the very first frame (Row 0)
#     row = df.iloc[0]
    
#     # 1. Look up joint positions safely without printing the whole dictionary
#     state_key = "state" if "state" in df.columns else "observation.state"
#     if state_key in df.columns:
#         # Format the numbers nicely so they fit on one clean line
#         formatted_joints = [f"{x:.4f}" for x in row[state_key]]
#         print(f"✅ Found State Key:     '{state_key}'")
#         print(f"🤖 8-DoF Joint Vectors (Radians):\n   {formatted_joints}")
#     else:
#         # Safe fallback columns list
#         state_cols = [c for c in df.columns if "state" in c]
#         print(f"⚠️ State key missing. Found related columns: {state_cols}")

#     print("-"*60)

#     # 2. Extract and decode image bytes without printing the raw bytes data
#     image_keys = [col for col in df.columns if "image" in col]
#     print(f"📷 Embedded Image Stream Features: {image_keys}")
    
#     for img_key in image_keys:
#         img_data = row[img_key]
        
#         # If it's stored as bytes, process it quietly inside memory
#         if isinstance(img_data, (bytes, bytearray)):
#             nparr = np.frombuffer(img_data, np.uint8)
#             img_bgr = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            
#             if img_bgr is not None:
#                 print(f"   -> Successfully parsed image frame from '{img_key}' (Shape: {img_bgr.shape})")
#                 # Open an external graphical window to show the image cleanly
#                 cv2.imshow(f"Visual Feed: {img_key}", img_bgr)
        
#         elif isinstance(img_data, np.ndarray):
#             print(f"   -> '{img_key}' is an uncompressed numpy array (Shape: {img_data.shape})")

#     print("="*60)
#     print("💡 ACTION REQUIRED: Click on the pop-up camera windows and press ANY key to exit.")
#     print("="*60 + "\n")
    
#     cv2.waitKey(0)
#     cv2.destroyAllWindows()

# if __name__ == "__main__":
#     inspect_clean_format()

# check_dataset.py
from lerobot.common.datasets.lerobot_dataset import LeRobotDataset
from torch.utils.data import DataLoader
from collections import Counter

ds = LeRobotDataset("Chi-chu/multitask_pick_and_place2")
print(f"Total frames: {len(ds)}")

# Check task label distribution
tasks = [ds[i]["task"] for i in range(len(ds))]
print("\nTask distribution:")
for task, count in Counter(tasks).items():
    print(f"  '{task}': {count} frames")

# Check a few samples for shape and content
print("\nSample frames:")
for i in [0, len(ds)//2, len(ds)-1]:
    sample = ds[i]
    print(f"  [{i}] task='{sample['task']}' | state={sample['state'].shape} | action={sample['actions'].shape}")