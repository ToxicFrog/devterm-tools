# ~/.profile: executed by Bourne-compatible login shells.

nixflake --random
if [ "$BASH" ]; then
  if [[ $(tty) != /dev/tty1 && $(tty) == /dev/tty* ]]; then
    sleep 2
    exec env YAFT=clockwise yaft
  fi
  if [ -f ~/.bashrc ]; then
    . ~/.bashrc
  fi
fi
