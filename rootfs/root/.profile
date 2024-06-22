# ~/.profile: executed by Bourne-compatible login shells.

export "TERMINFO_DIRS=/nix/var/nix/profiles/default/share/terminfo:$TERMINFO_DIRS"

if [ "$BASH" ]; then
  if [[ $(tty) != /dev/tty1 && $(tty) == /dev/tty* ]]; then
    exec env YAFT=clockwise yaft /bin/zsh
  fi
  nixflake --random
  if [ -f ~/.bashrc ]; then
    . ~/.bashrc
  fi
fi
