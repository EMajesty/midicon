{
  description = "MIDIcon firmware development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.mkShell {
            packages = [
              pkgs.platformio-core
              pkgs.clang-tools
            ];

            shellHook = ''
              export PLATFORMIO_CORE_DIR="$PWD/.pio-core"

              echo "MIDIcon firmware development shell"
              echo "  Build:        pio run -d firmware"
              echo "  Upload:       pio run -d firmware --target upload"
              echo "  Monitor:      pio device monitor -d firmware"
              echo "  Refresh LSP:  pio run -d firmware --target compiledb"
            '';
          };
        }
      );
    };
}

