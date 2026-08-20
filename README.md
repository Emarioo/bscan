
Experimenting with body tracking using normal webcam.

Tracking is trash at the moment.

![](docs/img/base_skeleton.png)

# Prerequisites
- Webcam
- 


# How to setup

First on the webcam turn off auto focus and light balancing and any other automatic features.

Then build with instructions belows if there is no release on github.
If there is a release untar the file and run bscan program.

NixOS
```bash
nix-shell

./package.py

source env.sh
bscan
```

Ubuntu
```bash
sudo apt install gcc python3 make 
# (may have some raylib, glfw problems... haven't tested properly)

./package.py

source env.sh
bscan
```

Windows
```bash
package.py

env.bat
bscan
```

# How to setup VR tracker driver

This is intended for SteamVR (tracker driver for OpenVR).
First in steam vr binaries find vrpathreg and add to environment variable PATH.
Or call exe with the absolute path name instead.

Below we tell SteamVR where to find the driver.

Windows
```bash
vrpathreg adddriver %cd%/releases/bscan-0.0.1-linux-x86_64/driver
```

Linux
```bash
vrpathreg adddriver $PWD/releases/bscan-0.0.1-linux-x86_64/driver
```

Now start SteamVR go into VR settings, turn on advanced settings, set the role for each tracker.

I don't know which is which myself yet. This part is work in progress.
Tracking sucks right now anyway so there's no point to do this.

