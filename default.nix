let
  pkgs = import <nixpkgs> {};
  nixpkgs-1703 = import (pkgs.fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs";
    rev = "1849e695b00a54cda86cb75202240d949c10c7ce";
    sha256 = "1fw9ryrz1qzbaxnjqqf91yxk1pb9hgci0z0pzw53f675almmv9q2";
  }) { };
  # TODO(RostakaGmfun): Use gcc7 from unstable pkgs
  gcc = pkgs.gcc6;
in
  { pkgs ? nixpkgs-1703, build-gce-image ? false }:
  let
    unity = pkgs.callPackage ./nix/unity.nix {};
    mini-printf = pkgs.callPackage ./nix/mini-printf.nix {};
    image-build-step = ''
      echo "Verifying if binary conforms GRUB Multiboot2"
      grub-file --is-x86-multiboot2 $out/bin/otrix_kernel
      mkdir -p build/isofiles/boot/grub
      mkdir $out/iso
      cp $out/bin/otrix_kernel build/isofiles/boot/
      cp $src/arch/x86_64/grub.cfg build/isofiles/boot/grub/
      grub-mkrescue -o otrix.iso build/isofiles
      cp otrix.iso $out/iso
    '' + (if build-gce-image then ''
      truncate -s1G otrix.iso
      mkdir -p $out/gce
      mv otrix.iso $out/gce/disk.raw
    '' else "");
  in with pkgs; stdenv.mkDerivation {
    name = "otrix";
    src = ./.;
    hardeningDisable = ["stackprotector"];
    buildInputs = [cmake gcc nasm grub2 xorriso kvm unity mini-printf];
    fixupPhase = " ";
    doCheck = true;
    checkTarget = "test";
    postInstall = image-build-step;
  }
