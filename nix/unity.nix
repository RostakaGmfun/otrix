{ stdenv, fetchFromGitHub, gcc }:
stdenv.mkDerivation {
    name = "Unity";
    src = fetchFromGitHub {
      owner = "ThrowTheSwitch";
      repo = "Unity";
      rev = "a868b2eb73ff36ef219383e5f93a30f82ec57eaa";
      sha256 = "0iir7m68hd02fhzynj0nx2xcqbi1ssjim6s42n3fs47vnazxj8b1";
      fetchSubmodules = true;
    };

    buildInputs = [gcc];
    phases = ["buildPhase" "installPhase"];
    buildPhase = ''
        gcc -c $src/src/unity.c $src/extras/fixture/src/unity_fixture.c -I$src/src/
        ar rcs libunity.a unity.o unity_fixture.o
    '';
    installPhase = ''
        mkdir -p $out/lib
        mkdir -p $out/include
        cp libunity.a $out/lib
        cp $src/src/*.h $out/include/
        cp $src/extras/fixture/src/*.h $out/include/
    '';
}
