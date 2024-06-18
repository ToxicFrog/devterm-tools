with import <unstable> {};
symlinkJoin {
  name = "devterm-profile";
  paths = with pkgsCross.riscv64; [
    chezmoi nb rlwrap stdmanpages hyfetch
    zsh eza btop w3m toilet figlet

    (atuin.overrideAttrs (_: {
      depsBuildBuild = [ buildPackages.protobuf ];
      buildInputs = [];
      preBuild = ''
        export PROTOC=${buildPackages.protobuf}/bin/protoc
        export PROTOC_INCLUDE=${protobuf}/include
      '';
    }))

    (yaft.overrideAttrs (_: {
      postInstall = ''
        mkdir -p $out/share
        cp glyph.h $out/share/
      '';
    }))

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

    (writeScriptBin "momovt" (builtins.readFile ./momovt))
    (writeScriptBin "colourtest" (builtins.readFile ../misc/eyecandy/colourtest))
    (writeScriptBin "nixflake" (builtins.readFile ../misc/eyecandy/nixflake))
    (writeScriptBin "devterm-deploy" ''
      ln -sf /lib/systemd/system/rc-local.service /etc/systemd/system/multi-user.target.wants
      rm -rf /etc/systemd/system/getty@tty1.service.d
      rsync -aPh ${./rootfs}/ /
      /etc/rc.local
      # apt remove landscape-sysinfo
    '')

    # needs a RISC-V JIT for javascript (?!): zellij
    # needs packaging: oscwrap
    # needs gnutls: taskwarrior
    # pick one of: vit taskwarrior-tui
  ];
}
