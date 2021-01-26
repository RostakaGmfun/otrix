let
  pkgs = import <nixpkgs> {};
in
  { build-gce-image ? false, toolchain-path, toolchain-prefix }:
  let
    unity = pkgs.callPackage ./nix/unity.nix {};
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
    buildInputs = [cmake gcc nasm grub2 xorriso kvm unity];
    fixupPhase = " ";
    cmakeFlags = ''-DCMAKE_TOOLCHAIN_FILE=${./.}/cmake/toolchain-x86.cmake
                   -DTOOLCHAIN_PATH=${toolchain-path} -DTOOLCHAIN_PREFIX=${toolchain-prefix}'';
    #doCheck = true;
    #checkTarget = "test";
    postInstall = image-build-step;
  }
