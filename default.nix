{ pkgs ? import <nixpkgs> {} }:
let
  unity = pkgs.callPackage ./nix/unity.nix {};
in with pkgs; stdenv.mkDerivation {
  name = "otrix";
  buildInputs = [cmake gcc nasm grub2 xorriso unity];
}
