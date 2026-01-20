import os
import sys
import subprocess
from script.utils import run_cmd, EXEC, CERT_DIR

DAEMON_NAME = "nf-server"
EXEC_INSTALL_PATH = f"/usr/bin/{DAEMON_NAME}"
CERT_INSTALL_PATH = f"/etc/nf/cert"
SERVICE_PATH = f"/etc/systemd/system/{DAEMON_NAME}.service"

SERVICE_TEXT = f"""[Unit]
Description=nf server service
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
ExecStart={EXEC_INSTALL_PATH} --database=false
Restart=no
RestartSec=3
User=root
LimitNOFILE=65535
StandardOutput=null
StandardError=null
SyslogIdentifier={DAEMON_NAME}
LimitCORE=infinity

[Install]
WantedBy=multi-user.target
"""


def install_certs():
    print("[*] Installing TLS certificates...")

    if not os.path.isdir(CERT_DIR):
        print(f"[*] Error: cert directory not found: {CERT_DIR}")
        sys.exit(1)

    run_cmd(["mkdir", "-p", CERT_INSTALL_PATH],
            msg="Creating /etc/nf/cert directory")

    for fname in os.listdir(CERT_DIR):
        src = os.path.join(CERT_DIR, fname)
        dst = os.path.join(CERT_INSTALL_PATH, fname)

        if not os.path.isfile(src):
            continue

        run_cmd(["cp", src, dst],
                msg=f"Copying cert file: {fname}")

        if fname.endswith(".key") or "key" in fname.lower():
            run_cmd(["chmod", "600", dst])
        else:
            run_cmd(["chmod", "644", dst])


def run():
    print(f"[*] Installing and starting {DAEMON_NAME} service...")

    if not os.path.isfile(EXEC):
        print(f"[*] Error: Built binary not found at: {EXEC}")
        print("[*] Please run './nf-server build' first.")
        sys.exit(1)

    status_result = subprocess.run(
        ["systemctl", "is-active", "--quiet", DAEMON_NAME],
        check=False,
        capture_output=True
    )

    if status_result.returncode == 0:
        run_cmd(["systemctl", "stop", DAEMON_NAME], msg="Stopping currently running service")

    run_cmd(["cp", EXEC, EXEC_INSTALL_PATH], msg="Copying binary to /usr/bin")
    run_cmd(["chmod", "755", EXEC_INSTALL_PATH])

    install_certs()

    print("[*] Writing systemd service file ...")
    try:
        with open(SERVICE_PATH, "w") as f:
            f.write(SERVICE_TEXT)
    except IOError as e:
        print(f"[*] Error, Failed to write service file: {e}")
        sys.exit(1)

    run_cmd(["systemctl", "daemon-reload"], msg="Reloading systemd")
    run_cmd(["systemctl", "restart", DAEMON_NAME], msg="Starting or restarting service")

    subprocess.run(["systemctl", "status", DAEMON_NAME, "--no-pager"])

    print("[*] Done, Service is active and running.")
