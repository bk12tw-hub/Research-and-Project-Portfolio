#!/usr/bin/env python3
"""
ros2_policy_client.py — Python 3.12 (system ROS2)

Uses official openpi_client WebSocket API instead of raw HTTP.
Continuous real-time control loop at 10Hz.

Usage:
  # Terminal 1 — start policy server (Python 3.11 openpi venv)
  cd /home/lenovo/openpi/openpi
  source .venv/bin/activate
  uv run scripts/serve_policy.py policy:checkpoint \
    --policy.config=pi0_autolife \
    --policy.dir=/home/lenovo/openpi/openpi/checkpoints/pi0_autolife/my_experiment/1000

  # Terminal 2 — Gazebo
  ros2 launch my_autolife1 gazebo_headless.launch.py

  # Terminal 3 — this script (Python 3.12)
  source /opt/ros/jazzy/setup.bash
  pip3 install openpi-client  # install once
  python3 ros2_policy_client.py
"""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState, Image
from trajectory_msgs.msg import JointTrajectory, JointTrajectoryPoint
from builtin_interfaces.msg import Duration
from cv_bridge import CvBridge

import numpy as np
import cv2
import time
import threading

# ── official openpi client ────────────────────────────────────
# install with: pip3 install openpi-client
# or: pip3 install git+https://github.com/Physical-Intelligence/openpi.git#subdirectory=packages/openpi-client
from openpi_client import image_tools
from openpi_client import websocket_client_policy

# ── robot config ──────────────────────────────────────────────
JOINT_NAMES = [
    "Joint_Left_Shoulder_Inner",
    "Joint_Left_Shoulder_Outer",
    "Joint_Left_UpperArm",
    "Joint_Left_Elbow",
    "Joint_Left_Forearm",
    "Joint_Left_Wrist_Upper",
    "Joint_Left_Wrist_Lower",
]
GRIPPER_JOINT      = "Joint_Left_Gripper"
CONTROL_HZ         = 10.0
CONTROL_DT         = 1.0 / CONTROL_HZ
POLICY_SERVER_HOST = "localhost"
POLICY_SERVER_PORT = 8000
TASK_PROMPT        = "pick up the cup"

# ── safety config ─────────────────────────────────────────────
MAX_DELTA_PER_STEP = 0.05   # max joint delta per step (radians)
MAX_GRIPPER_DELTA  = 0.1    # max gripper delta per step

# ── re-query every N steps (execute chunk open-loop between) ──
REQUERY_EVERY_N_STEPS = 25  # re-query halfway through 50-step chunk


class ROS2PolicyClient(Node):
    def __init__(self):
        super().__init__("ros2_policy_client")
        self.bridge = CvBridge()

        # ── robot state ───────────────────────────────────────
        self.current_joints = np.zeros(7)
        self.gripper_pos    = np.zeros(1)
        self.head_image     = np.zeros((224, 224, 3), dtype=np.uint8)
        self.wrist_image    = np.zeros((224, 224, 3), dtype=np.uint8)
        self.joints_ready   = False
        self.image_ready    = False

        # ── execution state ───────────────────────────────────
        self.action_chunk = None
        self.chunk_step   = 0
        self.lock         = threading.Lock()

        # ── stats ─────────────────────────────────────────────
        self.inference_count  = 0
        self.total_steps_sent = 0
        self.start_time       = time.time()

        # ── subscribers ───────────────────────────────────────
        self.create_subscription(
            JointState, "/joint_states",
            self.joint_callback, 10)
        self.create_subscription(
            Image, "/world/my_autolife1_gazebo_world/model/my_robot/link/Link_Neck_Yaw_to_Head/sensor/Camera_Head_Forehead/image",
            self.head_image_callback, 10)
        self.create_subscription(
            Image, "/world/my_autolife1_gazebo_world/model/my_robot/link/Link_Left_Wrist_Lower_to_Gripper/sensor/Camera_Gripper_Left/image",
            self.wrist_image_callback, 10)

        # ── publisher ─────────────────────────────────────────
        self.traj_pub = self.create_publisher(
            JointTrajectory,
            "/Left_arm_controller/joint_trajectory",
            10)

        # ── connect to policy server via WebSocket ────────────
        self.get_logger().info(
            f"Connecting to policy server "
            f"{POLICY_SERVER_HOST}:{POLICY_SERVER_PORT}...")
        try:
            self.client = websocket_client_policy.WebsocketClientPolicy(
                host=POLICY_SERVER_HOST,
                port=POLICY_SERVER_PORT)
            self.get_logger().info(" Connected to policy server!")
        except Exception as e:
            self.get_logger().error(
                f" Cannot connect to policy server: {e}\n"
                f"   Start serve_policy.py first!")
            raise

        # ── 10Hz control loop ─────────────────────────────────
        self.create_timer(CONTROL_DT, self.control_loop)
        self.get_logger().info(
            f"Running! Hz={CONTROL_HZ} | "
            f"requery_every={REQUERY_EVERY_N_STEPS} | "
            f"prompt='{TASK_PROMPT}'")

    # ── sensor callbacks ──────────────────────────────────────

    def joint_callback(self, msg: JointState):
        name_to_pos         = dict(zip(msg.name, msg.position))
        self.current_joints = np.array([
            name_to_pos.get(j, 0.0) for j in JOINT_NAMES])
        self.gripper_pos    = np.array([
            name_to_pos.get(GRIPPER_JOINT, 0.0)])
        self.joints_ready   = True

    def head_image_callback(self, msg: Image):
        try:
            img             = self.bridge.imgmsg_to_cv2(msg, "rgb8")
            self.head_image = cv2.resize(img, (224, 224))
            self.image_ready = True
        except Exception as e:
            self.get_logger().error(f"Head image error: {e}")

    def wrist_image_callback(self, msg: Image):
        try:
            img              = self.bridge.imgmsg_to_cv2(msg, "rgb8")
            self.wrist_image = cv2.resize(img, (224, 224))
        except Exception as e:
            self.get_logger().warn(f"Wrist image error: {e}")

    # ── main control loop ─────────────────────────────────────

    def control_loop(self):
        """Called at 10Hz — query policy and execute actions."""

        if not self.joints_ready:
            self.get_logger().info(
                "Waiting for /joint_states...",
                throttle_duration_sec=3.0)
            return
        if not self.image_ready:
            self.get_logger().info(
                "Waiting for /camera/head/image...",
                throttle_duration_sec=3.0)
            return

        with self.lock:
            # query new chunk when:
            # - no chunk yet
            # - chunk exhausted
            # - every REQUERY_EVERY_N_STEPS steps (open-loop execution)
            need_query = (
                self.action_chunk is None or
                self.chunk_step >= len(self.action_chunk) or
                self.chunk_step % REQUERY_EVERY_N_STEPS == 0
            )

            if need_query:
                self._query_policy()

            # execute current step
            if self.action_chunk is not None:
                step = self.chunk_step % len(self.action_chunk)
                self._execute_step(self.action_chunk[step])
                self.chunk_step       += 1
                self.total_steps_sent += 1

    # ── policy query using official openpi_client ─────────────

    def _query_policy(self):
        t_start = time.time()

        # ── build observation ──────────────────────────────────
        # use image_tools to match training preprocessing exactly
        observation = {
            "observation/image":
                image_tools.convert_to_uint8(
                    image_tools.resize_with_pad(
                        self.head_image, 224, 224)),

            "observation/wrist_image":
                image_tools.convert_to_uint8(
                    image_tools.resize_with_pad(
                        self.wrist_image, 224, 224)),

            "observation/state":
                np.concatenate([
                    self.current_joints,
                    self.gripper_pos
                ]).astype(np.float32),  # (8,)
            # unnormalized state — server handles normalization
            # "observation/joint_position":
            #     self.current_joints.astype(np.float32),
            # "observation/joint_velocity":
            #     np.zeros(7, dtype=np.float32),
            # "observation/gripper_position":
            #     self.gripper_pos.astype(np.float32),

            "prompt": TASK_PROMPT,
        }

        # ── call server ────────────────────────────────────────
        try:
            result       = self.client.infer(observation)
            action_chunk = result["actions"]  # (horizon, action_dim)
        except Exception as e:
            self.get_logger().error(f"Inference failed: {e}")
            return

        t_inf = time.time() - t_start
        self.inference_count += 1
        self.action_chunk     = action_chunk
        self.chunk_step       = 0

        self.get_logger().info(
            f"#{self.inference_count} | "
            f"shape={action_chunk.shape} | "
            f"time={t_inf:.3f}s | "
            f"step0={action_chunk[0].round(3)}")

    # ── action execution ──────────────────────────────────────

    def _execute_step(self, action: np.ndarray):
        # # safety clip
        # delta  = np.clip(action[:7], -MAX_DELTA_PER_STEP, MAX_DELTA_PER_STEP)
        # grip   = np.clip(action[7:8], -MAX_GRIPPER_DELTA, MAX_GRIPPER_DELTA)

        # # absolute positions
        # joints  = self.current_joints + delta
        # gripper = np.clip(self.gripper_pos + grip, 0.0, 1.0)

        # # publish
        # msg             = JointTrajectory()
        # msg.joint_names = JOINT_NAMES + [GRIPPER_JOINT]
        # pt              = JointTrajectoryPoint()
        # pt.positions    = joints.tolist() + gripper.tolist()
        # pt.velocities   = [0.0] * 8
        # pt.time_from_start = Duration(sec=0, nanosec=int(CONTROL_DT * 1e9))
        # msg.points      = [pt]
        # self.traj_pub.publish(msg)
        # ----------------------------------------------------------
        # msg             = JointTrajectory()
        # msg.joint_names = JOINT_NAMES
        # current_joints  = np.zeros(7)
        # gripper         = np.zeros(1)

        # for i, action in enumerate(self.action_chunk):
        #     delta_joints   = np.clip(action[:7], -0.05, 0.05)  # safety clip!
        #     current_joints = current_joints + delta_joints
        #     gripper        = np.clip(gripper + action[7:8], 0.0, 1.0)

        #     point = JointTrajectoryPoint()
        #     point.positions       = current_joints.tolist() + gripper.tolist()
        #     point.time_from_start = Duration(sec=i + 1, nanosec=0)
        #     msg.points.append(point)

        # time.sleep(1.0)
        # self.traj_pub.publish(msg)
        # --------------------------------------------------
            # safety check — limit how far we move per step
        target_joints = action[:7]

        # clip maximum movement per step from current position
        delta         = target_joints - self.current_joints
        clipped_delta = np.clip(delta, -MAX_DELTA_PER_STEP, MAX_DELTA_PER_STEP)
        next_joints   = self.current_joints + clipped_delta

        # gripper — keep current if not in action
        next_gripper  = self.gripper_pos

        # publish
        msg             = JointTrajectory()
        msg.joint_names = JOINT_NAMES + [GRIPPER_JOINT]
        pt              = JointTrajectoryPoint()
        pt.positions    = next_joints.tolist() + next_gripper.tolist()
        pt.velocities   = [0.0] * 8
        pt.time_from_start = Duration(sec=0, nanosec=int(CONTROL_DT * 1e9))
        msg.points      = [pt]
        self.traj_pub.publish(msg)

        self.get_logger().info(
            f"Target: {target_joints.round(3)} | "
            f"Current: {self.current_joints.round(3)} | "
            f"Sending: {next_joints.round(3)}",
            throttle_duration_sec=1.0)

    # ── status + shutdown ─────────────────────────────────────

    def print_status(self):
        elapsed = time.time() - self.start_time
        self.get_logger().info(
            f"\n{'='*50}\n"
            f"Runtime        : {elapsed:.0f}s\n"
            f"Inferences     : {self.inference_count}\n"
            f"Steps sent     : {self.total_steps_sent}\n"
            f"Joints         : {self.current_joints.round(3)}\n"
            f"Gripper        : {self.gripper_pos[0]:.3f}\n"
            f"{'='*50}")

    def shutdown(self):
        """Hold current position on shutdown."""
        msg             = JointTrajectory()
        msg.joint_names = JOINT_NAMES + [GRIPPER_JOINT]
        pt              = JointTrajectoryPoint()
        pt.positions    = (self.current_joints.tolist() +
                           self.gripper_pos.tolist())
        pt.velocities      = [0.0] * 8
        pt.time_from_start = Duration(sec=1, nanosec=0)
        msg.points         = [pt]
        self.traj_pub.publish(msg)
        time.sleep(0.5)


def main():
    rclpy.init()
    node = ROS2PolicyClient()

    def status_loop():
        while rclpy.ok():
            time.sleep(10.0)
            node.print_status()
    threading.Thread(target=status_loop, daemon=True).start()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.print_status()
        node.shutdown()
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()

""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

#!/usr/bin/env python3
# ros2_publisher.py  ← run with system python, NOT openpi venv

# import rclpy
# from trajectory_msgs.msg import JointTrajectory, JointTrajectoryPoint
# from builtin_interfaces.msg import Duration
# import numpy as np
# import time

# from openpi_client import image_tools
# from openpi_client import websocket_client_policy

# client = websocket_client_policy.WebsocketClientPolicy(host="localhost", port=8000)

# JOINT_NAMES = [
#     "Joint_Left_Shoulder_Inner",
#     "Joint_Left_Shoulder_Outer",
#     "Joint_Left_UpperArm",
#     "Joint_Left_Elbow",
#     "Joint_Left_Forearm",
#     "Joint_Left_Wrist_Upper",
#     "Joint_Left_Wrist_Lower",
#     "Joint_Left_Gripper",
# ]

# # load action chunk saved by inference script
# action_chunk = np.load("/tmp/action_chunk.npy")
# print(f"Loaded action_chunk: {action_chunk.shape}")

# rclpy.init()
# node = rclpy.create_node("vla_publisher")
# pub  = node.create_publisher(
#     JointTrajectory,
#     "/Left_arm_controller/joint_trajectory",
#     10)

# msg             = JointTrajectory()
# msg.joint_names = JOINT_NAMES
# current_joints  = np.zeros(7)
# gripper         = np.zeros(1)

# for i, action in enumerate(action_chunk):
#     delta_joints   = np.clip(action[:7], -0.05, 0.05)  # safety clip!
#     current_joints = current_joints + delta_joints
#     gripper        = np.clip(gripper + action[7:8], 0.0, 1.0)

#     point = JointTrajectoryPoint()
#     point.positions       = current_joints.tolist() + gripper.tolist()
#     point.time_from_start = Duration(sec=i + 1, nanosec=0)
#     msg.points.append(point)

# time.sleep(1.0)
# pub.publish(msg)
# print(f"Published {len(action_chunk)} steps → "
#       f"/Left_arm_controller/joint_trajectory")
# time.sleep(1.0)
# rclpy.shutdown()