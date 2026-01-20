import os
import subprocess
import sys
import shutil

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, os.pardir))

BUILD_DIR = os.path.join(ROOT_DIR, "build")
CERT_DIR = os.path.join(ROOT_DIR, "cert")

EXEC = os.path.join(BUILD_DIR, "nf-server")

INSTALL_ROOT = os.path.join(ROOT_DIR, "3rd_party", "install")

OPENSSL_VERSION = "3.2.0"
OPENSSL_DIR = os.path.join(ROOT_DIR, "3rd_party", "openssl")
OPENSSL_INSTALL = os.path.join(INSTALL_ROOT, "openssl")
OPENSSL_TAR = os.path.join(OPENSSL_DIR, f"openssl-{OPENSSL_VERSION}.tar.gz")
OPENSSL_SRC_PATH = os.path.join(OPENSSL_DIR, f"openssl-{OPENSSL_VERSION}")

SPDLOG_DIR = os.path.join(ROOT_DIR, "3rd_party", "spdlog")
SPDLOG_INSTALL = os.path.join(INSTALL_ROOT, "spdlog")

NUM_CORES = os.cpu_count() or 1
MAKE_JOBS = f"-j{NUM_CORES}"


def run_cmd(cmd, cwd=ROOT_DIR, msg=None):
    if msg:
        print(f"[*] {msg}...")
    try:
        process = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        while True:
            line = process.stdout.readline()
            if line:
                sys.stdout.write(line)
            if not line and process.poll() is not None:
                break

        if process.returncode != 0:
            print(f"[Error] Command failed with exit code {process.returncode}: {' '.join(cmd)}")
            sys.exit(process.returncode)

    except FileNotFoundError:
        print(f"[Error] Command not found. Check if '{cmd[0]}' is installed.")
        sys.exit(1)

    except Exception as e:
        print(f"[Error] An unexpected error occurred: {e}")
        sys.exit(1)
