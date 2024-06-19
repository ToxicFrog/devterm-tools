with import <unstable> {};
symlinkJoin {
  name = "devterm-profile";
  paths = with pkgsCross.riscv64; [
    chezmoi nb rlwrap stdmanpages input-utils termsonic
    zsh eza btop w3m toilet figlet

    (atuin.overrideAttrs (_: {
      depsBuildBuild = [ buildPackages.protobuf ];
      buildInputs = [];
      preBuild = ''
        export PROTOC=${buildPackages.protobuf}/bin/protoc
        export PROTOC_INCLUDE=${protobuf}/include
      '';
    }))

    yaft yaft.terminfo

    (micro.overrideAttrs (_: {
      postFixup = "";
    }))

    # Remove SDL2 dependency
    (sdl-jstest.overrideAttrs (_: {
      cmakeFlags = [ "-DBUILD_SDL2_JSTEST=OFF" ];
      buildInputs = [
        (SDL.override { libpulseaudio = null; })
        ncurses
      ];
    }))

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

    (writeScriptBin "momovt" (builtins.readFile ./momovt))
    (writeScriptBin "momovt-banner" (builtins.readFile ./momovt-banner))
    (writeScriptBin "devterm-gamepad-listener" (builtins.readFile ./devterm-gamepad-listener))
    (writeScriptBin "colourtest" (builtins.readFile ../misc/eyecandy/colourtest))
    (writeScriptBin "nixflake" (builtins.readFile ../misc/eyecandy/nixflake))
    (writeScriptBin "devterm-deploy" ''
      ln -sf ${zsh}/bin/zsh /bin/
      ln -sf /lib/systemd/system/rc-local.service /etc/systemd/system/multi-user.target.wants
      rm -rf /etc/systemd/system/getty@tty1.service.d
      rsync -aPh ${./rootfs}/ /
      /etc/rc.local
      . /root/.nix-profile/etc/profile.d/nix.sh
      mandb $(manpath -d)
      # apt remove landscape-sysinfo
    '')

    # needs a RISC-V JIT for javascript (?!): zellij
    # needs packaging: oscwrap
    # needs gnutls: taskwarrior

    # pick one of: vit taskwarrior-tui
  ];
}
