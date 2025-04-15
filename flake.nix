{
  description = "hush - a quiet and quaint shell in C";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
        };

        pname = "hush";
        version = "0.1.0";

        # Build the C project with CMake
        hushPkg = pkgs.stdenv.mkDerivation {
          inherit pname version;
          src = ./.;

          nativeBuildInputs = with pkgs; [ cmake clang ];

          buildInputs = [ ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
          ];

          installPhase = ''
            mkdir -p $out/bin
            cp hush $out/bin/
          '';
        };

        devShell = pkgs.mkShell {
          packages = with pkgs; [
            cmake
            clang
            clang-tools
            lldb
            gdb
            pkg-config
            valgrind
            bear
            glibc
          ];

          shellHook = ''
            echo "[+] Dev shell ready. Run 'cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1' to configure"
          '';
        };

      in {
        packages.default = hushPkg;
        devShells.default = devShell;
      });
}
