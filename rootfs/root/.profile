# ~/.profile: executed by Bourne-compatible login shells.

nixflake --random
export "TERMINFO_DIRS=/nix/var/nix/profiles/default/share/terminfo:$TERMINFO_DIRS"

if [ "$BASH" ]; then
  if [[ $(tty) != /dev/tty1 && $(tty) == /dev/tty* ]]; then
    sleep 2
    exec env YAFT=clockwise yaft /bin/zsh
  fi
  if [ -f ~/.bashrc ]; then
    . ~/.bashrc
  fi
fi
