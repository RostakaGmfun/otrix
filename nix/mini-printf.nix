{ stdenv, fetchFromGitHub, gcc }:
stdenv.mkDerivation {
    name = "mini-printf";
    src = fetchFromGitHub {
      owner = "mludvig";
      repo = "mini-printf";
      rev = "e8acf7fbbd768d3ae12cdcbd1bd6123e2078ec57";
      sha256 = "1h2yz7ap4z4vq9jb8y4pk72b0ppdrxi16m02v08ym0jdn5y20zbw";
      fetchSubmodules = true;
    };
    buildInputs = [gcc];
    phases = ["buildPhase" "installPhase"];
    buildPhase = ''
        gcc -c $src/mini-printf.c
        ar rcs libmini-printf.a mini-printf.o
    '';
    hardeningDisable = ["stackprotector"];
    installPhase = ''
        mkdir -p $out/lib
        mkdir -p $out/include
        cp libmini-printf.a $out/lib
        cp $src/mini-printf.h $out/include/
    '';
}
