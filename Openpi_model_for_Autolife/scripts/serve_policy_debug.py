import dataclasses
import enum
import logging
import socket

import tyro

from openpi.policies import policy as _policy
from openpi.policies import policy_config as _policy_config
from openpi.serving import websocket_policy_server
from openpi.training import config as _config

import cv2
import numpy as np
import json
import os
from datetime import datetime

#  validation ----------------------------------------------------------------------
class PolicyInferenceDebugger(_policy.Policy):
    """包裹原始 Policy，用來記錄每一幀的輸入與輸出。"""
    def __init__(self, policy: _policy.Policy, save_dir: str = "inference_debug"):
        self._policy = policy
        self.save_dir = save_dir
        os.makedirs(self.save_dir, exist_ok=True)
        self._step = 0

    def infer(self, observation: dict) -> dict:
        # 1. 執行推論
        result = self._policy.infer(observation)
        
        # 2. 存檔紀錄 (每步一個資料夾或前綴)
        timestamp = datetime.now().strftime("%H%M%S_%f")
        
        # 儲存主要影像 (Main Image)
        if "observation/image" in observation:
            img = observation["observation/image"]
            cv2.imwrite(os.path.join(self.save_dir, f"{self._step:04d}_{timestamp}_main.jpg"), 
                        cv2.cvtColor(img, cv2.COLOR_RGB2BGR))
            
        # 儲存手腕影像 (Wrist Image)
        if "observation/wrist_image" in observation:
            wrist_img = observation["observation/wrist_image"]
            cv2.imwrite(os.path.join(self.save_dir, f"{self._step:04d}_{timestamp}_wrist.jpg"), 
                        cv2.cvtColor(wrist_img, cv2.COLOR_RGB2BGR))
        
        # 儲存 Joint State 與 Output Action
        data_log = {
            "step": self._step,
            "timestamp": timestamp,
            "input_state": observation.get("observation/state", []).tolist() 
                           if isinstance(observation.get("observation/state"), np.ndarray) 
                           else observation.get("observation/state"),
            "output_actions": result.get("actions", []).tolist() 
                              if isinstance(result.get("actions"), np.ndarray) 
                              else result.get("actions")
        }
        
        with open(os.path.join(self.save_dir, f"{self._step:04d}_{timestamp}_log.json"), "w") as f:
            json.dump(data_log, f, indent=4)

        self._step += 1
        return result

    @property
    def metadata(self):
        return self._policy.metadata
#  -----------------------------------------------------------------------   

class EnvMode(enum.Enum):
    """Supported environments."""

    ALOHA = "aloha"
    ALOHA_SIM = "aloha_sim"
    DROID = "droid"
    LIBERO = "libero"


@dataclasses.dataclass
class Checkpoint:
    """Load a policy from a trained checkpoint."""

    # Training config name (e.g., "pi0_aloha_sim").
    config: str
    # Checkpoint directory (e.g., "checkpoints/pi0_aloha_sim/exp/10000").
    dir: str


@dataclasses.dataclass
class Default:
    """Use the default policy for the given environment."""


@dataclasses.dataclass
class Args:
    """Arguments for the serve_policy script."""

    # Environment to serve the policy for. This is only used when serving default policies.
    env: EnvMode = EnvMode.ALOHA_SIM

    # If provided, will be used in case the "prompt" key is not present in the data, or if the model doesn't have a default
    # prompt.
    default_prompt: str | None = None

    # Port to serve the policy on.
    port: int = 8000
    # Record the policy's behavior for debugging.
    record: bool = False

    # Specifies how to load the policy. If not provided, the default policy for the environment will be used.
    policy: Checkpoint | Default = dataclasses.field(default_factory=Default)


# Default checkpoints that should be used for each environment.
DEFAULT_CHECKPOINT: dict[EnvMode, Checkpoint] = {
    EnvMode.ALOHA: Checkpoint(
        config="pi05_aloha",
        dir="gs://openpi-assets/checkpoints/pi05_base",
    ),
    EnvMode.ALOHA_SIM: Checkpoint(
        config="pi0_aloha_sim",
        dir="gs://openpi-assets/checkpoints/pi0_aloha_sim",
    ),
    EnvMode.DROID: Checkpoint(
        config="pi05_droid",
        dir="gs://openpi-assets/checkpoints/pi05_droid",
    ),
    EnvMode.LIBERO: Checkpoint(
        config="pi05_libero",
        dir="gs://openpi-assets/checkpoints/pi05_libero",
    ),
}


def create_default_policy(env: EnvMode, *, default_prompt: str | None = None) -> _policy.Policy:
    """Create a default policy for the given environment."""
    if checkpoint := DEFAULT_CHECKPOINT.get(env):
        return _policy_config.create_trained_policy(
            _config.get_config(checkpoint.config), checkpoint.dir, default_prompt=default_prompt
        )
    raise ValueError(f"Unsupported environment mode: {env}")


def create_policy(args: Args) -> _policy.Policy:
    """Create a policy from the given arguments."""
    match args.policy:
        case Checkpoint():
            return _policy_config.create_trained_policy(
                _config.get_config(args.policy.config), args.policy.dir, default_prompt=args.default_prompt
            )
        case Default():
            return create_default_policy(args.env, default_prompt=args.default_prompt)


def main(args: Args) -> None:
    policy = create_policy(args)

    # --- 加入這段 HSU ---
    # 建立一個 debug 資料夾，名稱包含當前時間
    debug_folder = f"debug_log_{datetime.now().strftime('%Y%m%d_%H%M')}"
    policy = PolicyInferenceDebugger(policy, save_dir=debug_folder)
    logging.info(f"Inference Debugger active. Saving to {debug_folder}")
    # -------------------

    policy_metadata = policy.metadata

    # Record the policy's behavior.
    if args.record:
        policy = _policy.PolicyRecorder(policy, "policy_records")

    hostname = socket.gethostname()
    local_ip = socket.gethostbyname(hostname)
    logging.info("Creating server (host: %s, ip: %s)", hostname, local_ip)

    server = websocket_policy_server.WebsocketPolicyServer(
        policy=policy,
        host="0.0.0.0",
        port=args.port,
        metadata=policy_metadata,
    )
    server.serve_forever()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, force=True)
    main(tyro.cli(Args))
