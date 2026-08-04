#!/usr/bin/env python3

import sys, os, platform, shutil

ROOT = os.path.dirname(__file__)
DRIVER_ROOT = f"{os.path.dirname(__file__)}/driver"
BIN_DIR = f"bin"

RELEASES = f"{ROOT}/releases"

MAKE = "make" if platform.system() != "Windows" else "mingw32-make"

def main(args):
    VERSION = "0.0.1"
    PLATFORM = f"{platform.system()}-{platform.machine()}".lower()
    PACKAGE = f"{RELEASES}/bscan-{VERSION}-{PLATFORM}"
    DRIVER_DIR = f"{PACKAGE}/driver"

    if platform.system() == "Windows":
        BSCAN_EXE = "bscan.exe"
        DRIVER_DLL = "driver_bscan.dll"
        PACKAGE_ZIP = f"{RELEASES}/bscan-{VERSION}-{PLATFORM}.zip"
        DRIVER_BIN = f"{DRIVER_DIR}/bin/win64"
    else:
        BSCAN_EXE = "bscan"
        DRIVER_DLL = "libdriver_bscan.so"
        PACKAGE_ZIP = f"{RELEASES}/bscan-{VERSION}-{PLATFORM}.tar.gz"
        DRIVER_BIN = f"{DRIVER_DIR}/bin/linux64"

    if "clean" in args:
        err = os.system(f"{MAKE} -C {ROOT} clean")
        if err != 0:
            exit(1)

    err = os.system(f"{MAKE} -C {ROOT}")
    if err != 0:
        exit(1)

    os.makedirs(PACKAGE, exist_ok=True)
    os.makedirs(DRIVER_DIR, exist_ok=True)
    os.makedirs(f"{DRIVER_BIN}", exist_ok=True)

    shutil.copy(f"{BIN_DIR}/{BSCAN_EXE}", f"{PACKAGE}/{BSCAN_EXE}")
    shutil.copy(f"{BIN_DIR}/{DRIVER_DLL}", f"{DRIVER_BIN}/{DRIVER_DLL}")
    shutil.copy(f"{DRIVER_ROOT}/driver.vrdrivermanifest", f"{DRIVER_DIR}/driver.vrdrivermanifest")

    prev_dir = os.getcwd()
    os.chdir(RELEASES)

    if platform.system() == "Windows":
        err = os.system(f"7z a {PACKAGE_ZIP.replace("/","\\")} {os.path.basename(PACKAGE)}")
    else:
        err = os.system(f"tar -czf {PACKAGE_ZIP} {os.path.basename(PACKAGE)}")
    if err != 0:
        exit(1)

    os.chdir(prev_dir)

    print(f"\033[32mPackaged\033[0m {PACKAGE_ZIP}")

if __name__ == "__main__":
    main(sys.argv)


