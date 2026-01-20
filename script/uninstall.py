import os
import shutil

from script.utils import (
    INSTALL_ROOT, OPENSSL_TAR, OPENSSL_SRC_PATH, BUILD_DIR
)


def clean_build_dir():
    if os.path.exists(BUILD_DIR):
        shutil.rmtree(BUILD_DIR)
        print("[*] Build folder cleaned (3rd party installs preserved)")
    else:
        print("[*] Build folder already clean.")


def uninstall_dependencies():
    clean_build_dir()

    if os.path.exists(INSTALL_ROOT):
        shutil.rmtree(INSTALL_ROOT)
        print(f"[*] Uninstall: 3rd party installation folder '{INSTALL_ROOT}' removed.")
    else:
        print("[*] 3rd party installation folder already removed.")

    if os.path.exists(OPENSSL_SRC_PATH):
        shutil.rmtree(OPENSSL_SRC_PATH)
        print(f"[*] Uninstall: OpenSSL source folder '{OPENSSL_SRC_PATH}' removed.")

    if os.path.exists(OPENSSL_TAR):
        os.remove(OPENSSL_TAR)
        print(f"[*] Uninstall: OpenSSL tarball '{OPENSSL_TAR}' removed.")


def run():
    uninstall_dependencies()
    print("[*] All project and dependency files uninstalled successfully.")
