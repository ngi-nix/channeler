{
  description = "A reference implementation for a multi-channel and -link protocol for peer-to-peer communications.";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-21.05";
  inputs.liberate.url = "github:ngi-nix/liberate";
  inputs.liberate.inputs.nixpkgs.follows = "nixpkgs";

  outputs = { self, nixpkgs, liberate }:
    let
      liberateflake = liberate;

      # Generate a user-friendly version numer.
      version = builtins.substring 0 8 self.lastModifiedDate;

      # System types to support.
      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" "aarch64-darwin" ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = f: nixpkgs.lib.genAttrs supportedSystems (system: f system);

      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; overlays = [ self.overlay liberateflake.overlay ]; config.allowUnfree = true; });

      removeMesonWrap = path: name: libname:
        let
          meson-build = builtins.toFile "meson.build" ''
            project('${name}')

            compiler = meson.get_compiler('cpp')
            ${name}_dep = dependency('${libname}', required: false, allow_fallback: false)
            if not ${name}_dep.found()
              ${name}_dep = compiler.find_library('${libname}', required: true)
            endif
          '';
        in ''
          rm ./subprojects/${path}.wrap
          mkdir ./subprojects/${path}
          ln -s ${meson-build} ./subprojects/${path}/meson.build
        '';

      channeler =
        { stdenv, liberate, meson, ninja, gtest, lib }:
        stdenv.mkDerivation rec {
          name = "channeler-${version}";

          src = ./.;

          postPatch = ''
            echo > ./test/meson.build
          ''
            + removeMesonWrap "liberate" "liberate" "erate"
            + removeMesonWrap "gtest" "gtest" "gtest";

          buildInputs = [ gtest liberate ];
          nativeBuildInputs = [ meson ninja ];

          doCheck = true;

          meta = with lib; {
            description = "A reference implementation for a multi-channel and -link protocol for peer-to-peer communications.";
            longDescription = ''
              This library provides a reference implementation for a multi-channel and -link
              protocol for peer-to-peer communications.
              The rationale for this has several dimensions:

              NAT-piercing is prone to failure. Additionally, the number of available ports
              on a NAT limits how many peers behind the NAT can be served. To compensate
              for this, a multi-channel approach effectively allows multiplexing of
              independent "connections" (aka channels) along the same port.
              In a multi-link (or multi-home) device,
              e.g. on mobile devices, application connections should be kept stable even
              when the link technology changes (e.g. from WiFi to LTE, etc.)
              Finally, encryption parameters can be kept separate per channel, improving
              recovery times, when the encryption protocol is aware of both of the above.
            '';
            homepage    = "https://interpeer.io/";
            license     = licenses.unfree;
            platforms   = platforms.all;
          };
        };

    in

    {

      overlay = final: prev: {
        channeler = final.callPackage channeler {};
      };

      packages = forAllSystems (system:
        {
          inherit (nixpkgsFor.${system}) channeler;
        });

      defaultPackage = forAllSystems (system: self.packages.${system}.channeler);

      hydraJobs.channeler = self.defaultPackage;
    };
}
