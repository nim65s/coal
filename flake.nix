{
  description = "An extension of the Flexible Collision Library";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; } {
      systems = inputs.nixpkgs.lib.systems.flakeExposed;
      perSystem =
        { pkgs, self', ... }:
        {
          apps.default = {
            type = "app";
            program = pkgs.python3.withPackages (_: [ self'.packages.default ]);
          };
          devShells.default = with pkgs; mkShell {
            inputsFrom = [ self'.packages.default ];
            packages = let
              py = p: [
                p.boost
                p.eigenpy
                p.numpy
                p.scipy
              ];
            in [
              (python312.withPackages py)
              (python313.withPackages py)
            ];
          };
          packages = {
            default = self'.packages.coal;
            coal = pkgs.python3Packages.coal.overrideAttrs (_: {
              src = pkgs.lib.fileset.toSource {
                root = ./.;
                fileset = pkgs.lib.fileset.unions [
                  ./CMakeLists.txt
                  ./doc
                  ./hpp-fclConfig.cmake
                  ./include
                  ./package.xml
                  ./python
                  ./src
                  ./test
                ];
              };
            });
          };
        };
    };
}
