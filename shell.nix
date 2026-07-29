
{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  hardeningDisable = [ "fortify" ];

  buildInputs = with pkgs; [
    gcc
    python3
    gnumake

    raylib
    libx11
    glfw
    libGL
    xorg.libXrandr
    xorg.libXinerama
    xorg.libXcursor
    xorg.libXi
  ];
}
