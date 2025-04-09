{
  description = "hush; a quiet and quaint shell for the minimalist";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          name = "hush";
          src = ./.;
          
          nativeBuildInputs = with pkgs; [
            gcc
            cmake
            pkg-config
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_CXX_FLAGS=-O3"
          ];
        };

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            gcc
            cmake
            pkg-config
            gdb
            clang-tools # for clangd
          ];
        };
      });
}