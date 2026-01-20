import os
import sys
import subprocess
import shutil

from script.utils import (
    ROOT_DIR, INSTALL_ROOT, NUM_CORES, MAKE_JOBS, run_cmd,
    OPENSSL_VERSION, OPENSSL_DIR, OPENSSL_INSTALL, OPENSSL_TAR, OPENSSL_SRC_PATH,
    SPDLOG_DIR, SPDLOG_INSTALL
)


def install_openssl():
    libssl_path = os.path.join(OPENSSL_INSTALL, "lib64", "libssl.a")
    if os.path.exists(libssl_path):
        print("[*] OpenSSL already built and installed, skipping...")
        return

    os.makedirs(OPENSSL_DIR, exist_ok=True)
    os.makedirs(OPENSSL_INSTALL, exist_ok=True)

    if not os.path.exists(OPENSSL_TAR):
        print(f"[*] Downloading OpenSSL {OPENSSL_VERSION}...")
        try:
            subprocess.run(
                ["wget", f"https://www.openssl.org/source/openssl-{OPENSSL_VERSION}.tar.gz", "-O", OPENSSL_TAR],
                check=True)
        except subprocess.CalledProcessError:
            print("[Error] wget failed. Please check network connection or download the file manually.")
            sys.exit(1)
        except FileNotFoundError:
            print("[Error] wget command not found. Please install wget.")
            sys.exit(1)

    if not os.path.exists(OPENSSL_SRC_PATH):
        run_cmd(["tar", "xvf", OPENSSL_TAR, "-C", OPENSSL_DIR], cwd=OPENSSL_DIR, msg="Extracting OpenSSL source")

    config_cmd = ["./Configure", "linux-x86_64", "no-shared", f"--prefix={OPENSSL_INSTALL}"]
    run_cmd(config_cmd, cwd=OPENSSL_SRC_PATH, msg="Configuring OpenSSL")

    run_cmd(["make", MAKE_JOBS], cwd=OPENSSL_SRC_PATH, msg=f"Compiling OpenSSL with {NUM_CORES} jobs")
    run_cmd(["make", "install"], cwd=OPENSSL_SRC_PATH, msg="Installing OpenSSL")

    print("[*] OpenSSL installation complete.")


def install_spdlog():
    spdlog_config_path = os.path.join(SPDLOG_INSTALL, "lib", "cmake", "spdlog", "spdlogConfig.cmake")
    if os.path.exists(spdlog_config_path):
        print("[*] spdlog already installed, skipping...")
        return

    if not os.path.exists(os.path.join(SPDLOG_DIR, "CMakeLists.txt")):
        print(f"[!] spdlog source not found at {SPDLOG_DIR}. Please clone spdlog first.")
        sys.exit(1)

    spdlog_build_dir = os.path.join(SPDLOG_DIR, "build_temp")
    os.makedirs(spdlog_build_dir, exist_ok=True)
    os.makedirs(SPDLOG_INSTALL, exist_ok=True)

    cmake_cmd = ["cmake", SPDLOG_DIR, f"-DCMAKE_INSTALL_PREFIX={SPDLOG_INSTALL}",
                 "-DSPDLOG_BUILD_SHARED=OFF", "-DSPDLOG_BUILD_BENCH=OFF",
                 "-DSPDLOG_BUILD_EXAMPLES=OFF", "-DSPDLOG_BUILD_TESTS=OFF"]

    run_cmd(cmake_cmd, cwd=spdlog_build_dir, msg="Configuring spdlog with CMake")

    run_cmd(["make", MAKE_JOBS], cwd=spdlog_build_dir, msg=f"Compiling spdlog with {NUM_CORES} jobs")
    run_cmd(["make", "install"], cwd=spdlog_build_dir, msg="Installing spdlog")

    shutil.rmtree(spdlog_build_dir)
    print("[*] spdlog installation complete.")


def get_gpp_version():
    try:
        out = subprocess.check_output(["g++", "--version"], stderr=subprocess.STDOUT)
        first_line = out.decode().split("\n")[0]
        version_str = first_line.split()[-1]
        major = int(version_str.split(".")[0])
        return major
    except Exception:
        return 0


def install_gcc9():
    current = get_gpp_version()
    print(f"[*] Detected g++ major version: {current}")

    if current >= 9:
        print("[*] System g++ is already 9 or higher, skipping GCC toolchain installation.")
        return

    print("[*] Installing GCC 9 toolchain...")

    cmds = [
        ["sudo", "apt", "update"],
        ["sudo", "apt", "install", "-y", "software-properties-common"],
        ["sudo", "add-apt-repository", "-y", "ppa:ubuntu-toolchain-r/test"],
        ["sudo", "apt", "update"],
        ["sudo", "apt", "install", "-y", "g++-9"],
        ["sudo", "update-alternatives", "--install", "/usr/bin/g++", "g++", "/usr/bin/g++-9", "20"],
    ]

    for cmd in cmds:
        try:
            run_cmd(cmd, msg=f"Running: {' '.join(cmd)}")
        except Exception as e:
            print(f"[ERROR] Command failed: {cmd}\n{e}")
            sys.exit(1)

    print("[*] GCC version after update:")
    subprocess.run(["g++", "--version"], check=False)


def install_unixodbc():
    sql_header = "/usr/include/sql.h"
    if os.path.exists(sql_header):
        print("[*] unixODBC already installed, skipping...")
        return

    print("[*] Installing unixODBC (ODBC headers + runtime)...")

    cmds = [
        ["sudo", "apt", "update"],
        ["sudo", "apt", "install", "-y", "unixodbc", "unixodbc-dev"],
    ]

    for cmd in cmds:
        try:
            run_cmd(cmd, msg=f"Running: {' '.join(cmd)}")
        except Exception as e:
            print(f"[ERROR] unixODBC install failed: {cmd}\n{e}")
            sys.exit(1)

    if not os.path.exists(sql_header):
        print("[ERROR] sql.h not found after unixODBC installation.")
        sys.exit(1)

    print("[*] unixODBC installation complete.")


def install_build_essential():
    required_tools = ["make", "gcc", "g++", "cmake"]
    missing = []

    for tool in required_tools:
        if shutil.which(tool) is None:
            missing.append(tool)

    if not missing:
        print("[*] Build tools (make, gcc, g++, cmake) already installed, skipping...")
        return

    print(f"[*] Installing build tools: {', '.join(missing)}")

    cmds = [
        ["sudo", "apt", "update"],
        ["sudo", "apt", "install", "-y", "build-essential", "cmake"],
    ]

    for cmd in cmds:
        try:
            run_cmd(cmd, msg=f"Running: {' '.join(cmd)}")
        except Exception as e:
            print(f"[ERROR] build tools install failed: {cmd}\n{e}")
            sys.exit(1)

    # 최종 검증
    for tool in required_tools:
        if shutil.which(tool) is None:
            print(f"[ERROR] Required tool '{tool}' not found after installation.")
            sys.exit(1)

    print("[*] build-essential + cmake installation complete.")


def run():
    os.makedirs(INSTALL_ROOT, exist_ok=True)
    install_build_essential()
    install_openssl()
    install_spdlog()
    install_gcc9()
    install_unixodbc()
    print("[*] All dependencies installed successfully.")
