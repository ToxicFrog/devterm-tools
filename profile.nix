let
  unstable = import <unstable> {};
  pkgs = unstable.pkgsCross.riscv64;

  atuin = pkgs.atuin.overrideAttrs (_: with pkgs; {
    depsBuildBuild = [ buildPackages.protobuf ];
    buildInputs = [];
    preBuild = ''
      export PROTOC=${buildPackages.protobuf}/bin/protoc
      export PROTOC_INCLUDE=${protobuf}/include
    '';
  });

  micro = pkgs.micro.overrideAttrs (_: { postFixup = ""; });

  yaft = pkgs.yaft.overrideAttrs (old: with pkgs; {
    depsBuildBuild = [ buildPackages.stdenv.cc ];
    nativeBuildInputs = [ buildPackages.ncurses ];
    src = ./yaft;
  });

  scripts = with pkgs; [
    (writeScriptBin "momovt" (builtins.readFile ./momovt))
    (writeScriptBin "momovt-banner" (builtins.readFile ./momovt-banner))
    (writeScriptBin "devterm-gamepad-listener" (builtins.readFile ./devterm-gamepad-listener))
    (writeScriptBin "colourtest" (builtins.readFile ../misc/eyecandy/colourtest))
    (writeScriptBin "nixflake" (builtins.readFile ../misc/eyecandy/nixflake))
    (writeScriptBin "devterm-activate" ''
      env \
        NIX_ZSH=${pkgs.zsh} \
        NIX_ROOTFS=${./rootfs} \
        ${./devterm-activate}
    '')
  ];
in pkgs.symlinkJoin {
  name = "devterm-profile";
  paths = with pkgs; [
    atuin
    btop
    chezmoi
    eza
    figlet toilet
    input-utils
    micro
    nb
    rlwrap
    stdmanpages
    termsonic
    vivid
    w3m
    yaft yaft.terminfo
    zsh
    zsh.man
    zsh-autopair zsh-autosuggestions zsh-completions zsh-f-sy-h zsh-z
  ] ++ scripts;
}
    # Unfortunately builds taskchampion alongside task, which depends on Ring,
    # which does not support RISC-V.
    # (taskwarrior3.overrideAttrs (_: {
    #   # Remove xdg-open dependency
    #   postPatch = "";
    #   # Fix cross-compile
    #   buildInputs = [ libuuid ];
    #   cmakeFlags = [
    #     "-DRust_CARGO_TARGET=riscv64gc-unknown-linux-gnu"
    #     "--build" "build" "--target" "task_executable"
    #   ];
    #   preBuild = ''
    #     export TARGET_CC=${stdenv.cc}/bin/gcc
    #   '';
    # }))


    # needs a RISC-V JIT for javascript (?!): zellij
    # needs packaging: oscwrap
    # needs gnutls: taskwarrior

    # pick one of: vit taskwarrior-tui
