with import <unstable> {};
symlinkJoin {
  name = "devterm-profile";
  paths = with pkgsCross.riscv64; [
    chezmoi nb yaft rlwrap stdmanpages
    zsh eza btop w3m toilet figlet hyfetch

    (atuin.overrideAttrs (_: {
      depsBuildBuild = [ buildPackages.protobuf ];
      buildInputs = [];
      preBuild = ''
        export PROTOC=${buildPackages.protobuf}/bin/protoc
        export PROTOC_INCLUDE=${protobuf}/include
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

    # needs a RISC-V JIT for javascript (?!): zellij
    # needs packaging: oscwrap
    # needs gnutls: taskwarrior
    # pick one of: vit taskwarrior-tui
  ];
}
