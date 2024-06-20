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
    (writeScriptBin "devterm-deploy" ''
      ln -sf ${zsh}/bin/zsh /bin/
      ln -sf /lib/systemd/system/rc-local.service /etc/systemd/system/multi-user.target.wants
      rm -rf /etc/systemd/system/getty@tty1.service.d
      rsync -aPh ${./rootfs}/ /
      /etc/rc.local
      . /root/.nix-profile/etc/profile.d/nix.sh
      echo -n "Rebuilding man index: "
      mandb $(manpath -d 2>/dev/null) &>/dev/null
      echo "done."
      echo -n "Reloading services: "
      systemctl daemon-reload
      systemctl restart devterm-controls.service
      echo "done."
      # apt remove landscape-sysinfo
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
    w3m
    yaft yaft.terminfo
    zsh
    zsh.man
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
