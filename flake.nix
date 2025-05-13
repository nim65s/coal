{
  description = "An extension of the Flexible Collision Library";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    # TODO: switch back to nixos-unstable after
    # https://github.com/NixOS/nixpkgs/pull/395016
    nixpkgs.url = "github:NixOS/nixpkgs/refs/pull/395016/head";
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
          devShells.default = pkgs.mkShell { inputsFrom = [ self'.packages.default ]; };
          packages = {
            default = self'.packages.coal;
            coal = pkgs.python3Packages.coal.overrideAttrs (super: {
              postPatch = ''
                substituteInPlace python-nb/CMakeLists.txt --replace-fail \
                  "$""{Python_SITELIB}" \
                  "${pkgs.python3.sitePackages}"
              '';
              propagatedBuildInputs = super.propagatedBuildInputs ++ [
                pkgs.python3Packages.nanobind
                pkgs.python3Packages.nanoeigenpy
              ];
              pythonImportsCheck = [ "coal" ]; # hppfcl is broken with nanobind
              src = pkgs.lib.fileset.toSource {
                root = ./.;
                fileset = pkgs.lib.fileset.unions [
                  ./CMakeLists.txt
                  ./doc
                  ./hpp-fclConfig.cmake
                  ./include
                  ./package.xml
                  ./python
                  ./python-nb
                  ./src
                  ./test
                ];
              };
            });
          };
        };
    };
}
