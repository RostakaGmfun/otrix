{ pkgs ? import <nixpkgs> { } }:
let
    unity = pkgs.callPackage ./nix/unity.nix {};
in with pkgs; stdenv.mkDerivation {
        name = "otrix";
        src = ../otrix;
        buildInputs = [cmake gcc nasm grub2 xorriso kvm unity];
        fixupPhase = " ";
        doCheck = true;
        checkTarget = "test";
        postInstall = ''
            echo "Verifying if binary conforms GRUB Multiboot2"
            grub-file --is-x86-multiboot2 $out/bin/otrix_kernel
            mkdir -p build/isofiles/boot/grub
            mkdir $out/iso
            cp $out/bin/otrix_kernel build/isofiles/boot/
            cp $src/arch/x86_64/grub.cfg build/isofiles/boot/grub/
            grub-mkrescue -o otrix.iso build/isofiles
            cp otrix.iso $out/iso
        '';
    }
