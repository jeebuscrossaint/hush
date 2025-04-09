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
        packages.default = pkgs.buildMesonPackage {
          pname = "hush";
          version = "0.1.0";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            meson
            ninja
            pkg-config
          ];

          buildInputs = with pkgs; [
            # Add any runtime deps here (none yet)
          ];

          # Optional: If you have man pages, completion, etc.
          # installPhase = ''
          #   runHook preInstall
          #   mkdir -p $out/bin
          #   cp hush $out/bin/
          #   runHook postInstall
          # '';
        };

        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            gcc
            meson
            ninja
            pkg-config
            gdb
            clang-tools # for clangd
          ];
        };
      });
}
