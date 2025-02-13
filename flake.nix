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
          devShells.default =
            with pkgs;
            mkShell {
              inputsFrom = [ pkgs.python3Packages.coal ];
              packages =
                let
                  py = p: [
                    p.boost
                    p.eigenpy
                    p.numpy
                    p.scipy
                  ];
                in
                [
                  (python312.withPackages py)
                  (python313.withPackages py)
                ];
            };
          packages = {
            default = self'.packages.coal;
            coal = pkgs.coal.overrideAttrs (super: {
              cmakeFlags = super.cmakeFlags ++ [ "-DCOAL_DISABLE_HPP_FCL_WARNINGS=ON" ];
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
            py-coal =
              (self'.packages.coal.override {
                inherit (pkgs) python3Packages;
                pythonSupport = true;
              }).overrideAttrs
                (super: {
                  cmakeFlags = super.cmakeFlags ++ [
                    "-DBUILD_ONLY_PYTHON_PYTHON_INTERFACE=ON"
                  ];
                  propagatedBuildInputs = super.propagatedBuildInputs ++ [
                    self'.packages.coal
                  ];
                });
            py312-coal = self'.packages.py-coal.override {
              python3Packages = pkgs.python312Packages;
            };
            py313-coal = self'.packages.py-coal.override {
              python3Packages = pkgs.python313Packages;
            };
          };
        };
    };
}
