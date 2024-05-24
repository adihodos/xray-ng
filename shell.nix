let
  pkgs = import <nixpkgs> {};
in
#pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
pkgs.mkShell {
  buildInputs = with pkgs; [
    bashInteractive

    #gcc
    gdb
    lldb
    gdbgui
    gf
    renderdoc
    cmake    
    pkg-config
    python3Full
    kdiff3
    vulkan-tools
    vulkan-headers
    vulkan-loader
    vulkan-tools-lunarg
    vulkan-validation-layers
    vulkan-utility-libraries
    vulkan-caps-viewer
    vulkan-validation-layers
    shaderc
    glslang
    bashInteractive
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
  ];

  shellHook = ''
      export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$APPEND_LIBRARY_PATH"
    '';

  VK_LAYER_PATH = "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d";
}
  
