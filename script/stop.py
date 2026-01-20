import os
import subprocess
from script.utils import run_cmd

DAEMON_NAME = "nf-server"


def run():
    print(f"[*] Stopping {DAEMON_NAME} service ...")

    status_result = subprocess.run(
        ["systemctl", "is-active", "--quiet", DAEMON_NAME],
        check=False,
        capture_output=True
    )

    if status_result.returncode != 0:
        print(f"[*] {DAEMON_NAME} is not running or inactive.")
        return

    run_cmd(["systemctl", "stop", DAEMON_NAME], msg=f"{DAEMON_NAME} service stopped")

    subprocess.run(["systemctl", "status", DAEMON_NAME, "--no-pager"])

    print("[*] Done!")
