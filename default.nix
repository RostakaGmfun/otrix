{ pkgs ? import <nixpkgs> { } }:
    with pkgs; stdenv.mkDerivation {
        name = "otrix";
        buildInputs = [gcc nasm grub2 xorriso kvm];
        installTargets = "";
        src = ../otrix;
    }
