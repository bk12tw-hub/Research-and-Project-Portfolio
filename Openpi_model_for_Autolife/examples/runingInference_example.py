# import os
# os.environ["JAX_PLATFORMS"] = "cuda"
# os.environ["XLA_FLAGS"] = "--xla_gpu_enable_triton_gemm=false"


# from openpi.training import config as _config
# from openpi.policies import policy_config
# from openpi.shared import download
# import numpy as np

# config = _config.get_config("pi05_droid")
# checkpoint_dir = download.maybe_download("gs://openpi-assets/checkpoints/pi05_droid")

# # Create a trained policy.
# policy = policy_config.create_trained_policy(config, checkpoint_dir)

# # Run inference on a dummy example.
# example = {
#     "observation/exterior_image_1_left": np.zeros((224, 224, 3), dtype=np.uint8),
#     "observation/wrist_image_left": np.zeros((224, 224, 3), dtype=np.uint8),

#     # Robot state observations
#     "observation/joint_position": np.zeros(7, dtype=np.float64),      # 7 joint angles
#     "observation/joint_velocity": np.zeros(7, dtype=np.float64),      # 7 joint velocities
#     "observation/gripper_position": np.zeros(1, dtype=np.float64),    # gripper open/close

#     "prompt": "pick up the red cup"
# }
# action_chunk = policy.infer(example)["actions"]
# print("Shape:", action_chunk.shape)
# # print("Actions:", action_chunk)

# # save for ROS2 publisher to read
# np.save("/tmp/action_chunk.npy", action_chunk)
# print("Saved to /tmp/action_chunk.npy")
# """""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
# above is the original example
# """""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

# import os
# os.environ["JAX_PLATFORMS"] = "cuda"
# os.environ["XLA_FLAGS"] = "--xla_gpu_enable_triton_gemm=false"

# import numpy as np
# from openpi.training import config as _config
# from openpi.policies import policy_config
# from openpi.shared import download

# # ── read real joint states saved by read_joints.py ───────────
# state = np.load("/tmp/joint_states.npy")   # shape (8,)
# print(f"Loaded state: {state.round(4)}")

# # ── build observation ─────────────────────────────────────────
# example = {
#     "observation/image":       np.zeros((224, 224, 3), dtype=np.uint8),
#     "observation/wrist_image": np.zeros((224, 224, 3), dtype=np.uint8),
#     "observation/state":       state.astype(np.float64),
#     "prompt":                  "pick up the red cup",
# }

# # ── load model + run inference ────────────────────────────────
# print("Loading pi0_base...")
# config         = _config.get_config("pi0_base")
# checkpoint_dir = download.maybe_download("gs://openpi-assets/checkpoints/pi0_base")
# policy         = policy_config.create_trained_policy(config, checkpoint_dir)

# action_chunk = policy.infer(example)["actions"]
# print(f"Action chunk shape: {action_chunk.shape}")
# print(f"First step delta joints : {action_chunk[0, :7].round(4)}")
# print(f"First step gripper      : {action_chunk[0, 7]:.4f}")

# # ── save for ROS2 publisher ───────────────────────────────────
# np.save("/tmp/action_chunk.npy", action_chunk)
# print("Saved! Now run: python3 ros2_publisher.py")

# """""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

# """""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
import os
os.environ["JAX_PLATFORMS"] = "cuda"
os.environ["XLA_FLAGS"] = "--xla_gpu_enable_bf16_6way_gemm=true"
os.environ["XLA_PYTHON_CLIENT_PREALLOCATE"] = "false"
os.environ["XLA_PYTHON_CLIENT_MEM_FRACTION"] = "0.75"  # 75% of VRAM only

# ── limit XLA compilation memory ─────────────────────────────
os.environ["TF_CPP_MIN_LOG_LEVEL"] = "3"
os.environ["XLA_CLIENT_MEM_USAGE_HARD_LIMIT_GB"] = "80"  # cap at 80GB

import numpy as np
from openpi.training import config as _config
from openpi.policies import policy_config
from openpi.shared import download


def load_sensor_data():
    """Load real sensor data saved by read_sensors.py"""

    # ── joint states ───────────────────────────────────────────
    try:
        state = np.load("/tmp/joint_states.npy")
        print(f" Joint states  : {state.round(4)}")
    except FileNotFoundError:
        print(" /tmp/joint_states.npy not found — run read_sensors.py first!")
        state = np.zeros(8, dtype=np.float64)

    # ── head image ─────────────────────────────────────────────
    try:
        head_image = np.load("/tmp/head_image.npy")
        print(f" Head image    : {head_image.shape} "
              f"min={head_image.min()} max={head_image.max()}")
        if head_image.min() == head_image.max():
            print(" Head image is uniform — camera may not be working!")
    except FileNotFoundError:
        print(" /tmp/head_image.npy not found — using zeros")
        head_image = np.zeros((224, 224, 3), dtype=np.uint8)

    # ── wrist image — zeros placeholder (not used yet) ────────
    wrist_image = np.zeros((224, 224, 3), dtype=np.uint8)
    print(f" Wrist image   : zeros placeholder (not used yet)")

    return state, head_image, wrist_image


# ── Step 1: load real sensor data ────────────────────────────
print("=" * 50)
print("Loading sensor data from Gazebo...")
print("=" * 50)
state, head_image, wrist_image = load_sensor_data()

# ── Step 2: build observation ─────────────────────────────────
example = {
    "observation/exterior_image_1_left": head_image.astype(np.uint8),        # real head camera
    "observation/wrist_image_left":      wrist_image.astype(np.uint8),  # zeros for now
    # "observation/state":                 state.astype(np.float32), 
    "observation/joint_velocity":        np.zeros(7, dtype=np.float64),
    "observation/joint_position":        state[:7].astype(np.float64),       # real joint positions
    "observation/gripper_position":      state[7:8].astype(np.float64),      # real gripper

    "prompt": "pick up the red cup",
}

print(f"\nFeeding to model:")
# print(f"  state       : {example['observation/state'].round(4)}")
print(f"  head image  : {example['observation/exterior_image_1_left'].shape}")
print(f"  wrist image : {example['observation/wrist_image_left'].shape}")
print(f"  prompt      : {example['prompt']}")

# ── Step 3: load model ────────────────────────────────────────
print("\n" + "=" * 50)
print("Loading pi0_base model...")
print("=" * 50)
config         = _config.get_config("pi05_droid")
checkpoint_dir = download.maybe_download("gs://openpi-assets/checkpoints/pi05_droid")
policy         = policy_config.create_trained_policy(config, checkpoint_dir)
print("Model loaded!")

# ── Step 4: run inference ─────────────────────────────────────
print("\nRunning inference...")
action_chunk = policy.infer(example)["actions"]

print("\n" + "=" * 50)
print("VLA Output:")
print("=" * 50)
print(f"Action chunk shape : {action_chunk.shape}")
print(f"\nFirst action step:")
print(f"  delta joints     : {action_chunk[0, :7].round(4)}")
print(f"  gripper command  : {action_chunk[0, 7]:.4f}")
print(f"\nFull action chunk (first 5 steps):")
for i in range(min(5, len(action_chunk))):
    print(f"  step {i}: joints={action_chunk[i, :7].round(3)} "
          f"gripper={action_chunk[i, 7]:.3f}")

# ── Step 5: save for ROS2 publisher ──────────────────────────
np.save("/tmp/action_chunk.npy", action_chunk)
print("\n Saved to /tmp/action_chunk.npy")
print("Now run: python3 ros2_publisher.py")