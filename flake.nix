{
  description = "A Nix-flake-based C/C++ development environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs = {
    self,
    nixpkgs,
  }: let
    supportedSystems = ["x86_64-linux"];
    forEachSupportedSystem = f:
      nixpkgs.lib.genAttrs supportedSystems (system:
        f {
          pkgs = import nixpkgs {
            inherit system;
            config.allowUnfree = true;
          };
        });
  in {
    devShells = forEachSupportedSystem ({pkgs}: {
      default =
        pkgs.mkShell.override {
          # Override stdenv in order to change compiler:
          # stdenv = pkgs.clangStdenv;
        }
        {
          packages = with pkgs; [
            gcc
            gdb
            seer
            libgit2
            libgit2.dev
            openssl
            #pwndbg
            #gef
            gf
            renderdoc
            cmake
            cmakeCurses
            cmake-format
            pkg-config

            vulkan-tools
            vulkan-headers
            vulkan-loader
            vulkan-tools-lunarg
            vulkan-validation-layers
            vulkan-utility-libraries
            vulkan-caps-viewer
            vulkan-validation-layers
            ktx-tools
            zed-editor

            shaderc
            shaderc.bin
            shaderc.static
            shaderc.dev
            shaderc.lib
            glslang
            bashInteractive
            python3

            qtcreator

            # (python311.withPackages (python-pkgs: [
            #   # select Python packages here
            #   python-pkgs.pwndbg
            #   #python-pkgs.pandas
            # ]))

            (vscode-with-extensions.override {
              vscodeExtensions = with vscode-extensions; [
                ms-vscode.cpptools
                vadimcn.vscode-lldb
                mkhl.direnv
              ];
            })

            # libs
            zlib
            xorg.libX11
            xorg.libX11.dev
            xorg.libXi
            xorg.libXcursor
            xorg.libXrandr
            xorg.libXext
            xorg.libXinerama
            xorg.libXrender
            xorg.libXxf86vm
            libxkbcommon
            libxkbcommon.dev
            xorg.libxcb
            xorg.libxcb.dev
            libGL
          ];

          APPEND_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
            pkgs.xorg.libXcursor
            pkgs.xorg.libXi
            pkgs.xorg.libX11
            pkgs.xorg.libXrandr
            pkgs.xorg.libXext
            pkgs.xorg.libXxf86vm
            pkgs.libxkbcommon
            pkgs.xorg.libxcb.dev
            pkgs.shaderc.lib
            pkgs.shaderc.dev
            pkgs.shaderc.static
            pkgs.glslang
          ];

          shellHook = ''
            export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$APPEND_LIBRARY_PATH"
          '';

          VK_LAYER_PATH = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
        };
    });
  };
}
