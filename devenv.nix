{ pkgs, lib, config, inputs, ... }:
let
  pkgs-unstable = import inputs.nixpkgs-unstable {
    system = pkgs.stdenv.system;
    config.allowUnfree = true; # Optional: adjust to match your needs
  };
in
{
  # https://devenv.sh/basics/
  env.GREET = "XRAY-NG-SLIMMED";

  # https://devenv.sh/packages/
  packages = [
    pkgs.git
    pkgs-unstable.gcc16
    pkgs.mold
    pkgs.nnd

    pkgs.vulkan-tools
    pkgs.vulkan-headers
    pkgs.vulkan-loader
    pkgs.vulkan-tools-lunarg
    pkgs.vulkan-utility-libraries
    pkgs.vulkan-caps-viewer
    pkgs.vulkan-validation-layers

    pkgs.shaderc
    pkgs.shaderc.dev
    pkgs.shaderc.static

    pkgs.xorg.libX11
    pkgs.xorg.libX11.dev
    pkgs.xorg.libXi
    pkgs.xorg.libXcursor
    pkgs.xorg.libXrandr
    pkgs.xorg.libXext
    pkgs.xorg.libXinerama
    pkgs.xorg.libxcb.dev

    pkgs.libxkbcommon

    pkgs.zstd.dev
  ];

  # https://devenv.sh/languages/
  # languages.rust.enable = true;

  # https://devenv.sh/processes/
  # processes.dev.exec = "${lib.getExe pkgs.watchexec} -n -- ls -la";

  # https://devenv.sh/services/
  # services.postgres.enable = true;

  # https://devenv.sh/scripts/
  scripts.hello.exec = ''
    echo Hello from $GREET
    mkdir -p temp
  '';

  # https://devenv.sh/basics/
  enterShell = ''
    hello         # Run scripts directly
    # git --version # Use packages
  '';

  # https://devenv.sh/tasks/
  # tasks = {
  #   "myproj:setup".exec = "mytool build";
  #   "devenv:enterShell".after = [ "myproj:setup" ];
  # };

  # https://devenv.sh/tests/
  enterTest = ''
    echo "Running tests"
    git --version | grep --color=auto "${pkgs.git.version}"
  '';

  # https://devenv.sh/git-hooks/
  # git-hooks.hooks.shellcheck.enable = true;

  # See full reference at https://devenv.sh/reference/options/
}
