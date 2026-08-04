
Experimenting with processing of camera data.

![](docs/img/base_skeleton.png)

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


# VR tracker driver

Windows
```bash
vrpathreg adddriver %cd%/releases/bscan-0.0.1-linux-x86_64/driver
```


Linux
```bash
vrpathreg adddriver $PWD/releases/bscan-0.0.1-linux-x86_64/driver
```

