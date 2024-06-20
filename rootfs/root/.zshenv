export ZDOTDIR="$HOME"
export TERMINFO_DIRS="/nix/var/nix/profiles/default/share/terminfo:$TERMINFO_DIRS"

# possible alternate colour scheme: %K{22}%F{10} for green rather than amber
export PS1=$'%F{11}%K{130}%n@%m %~%E%f%k\n%(!.$.$)%f '
export RPROMPT="%{$(echotc UP 1)%}%(?,,%K{1}%F{15}%B (exit %?%) )%K{130}%F{11} [%D{%F %T}]%f%k%b%{$(echotc DO 1)%}"

export LANG=en_US.UTF-8
# export LC_TIME=en_DK.UTF-8

export EDITOR=micro
export VISUAL=micro
export PAGER="less -R"

export HISTFILE="$ZDOTDIR/.zhistory"
export HISTSIZE=$((1024*1024))
export SAVEHIST=$((1024*1024))

typeset -A key
key[Home]="$terminfo[khome]"
key[End]="$terminfo[kend]"
key[Insert]="$terminfo[kich1]"
key[Backspace]="$terminfo[kbs]"
key[Delete]="$terminfo[kdch1]"
key[KUp]="$terminfo[kcuu1]"
key[KDown]="$terminfo[kcud1]"
key[KLeft]="$terminfo[kcub1]"
key[KRight]="$terminfo[kcuf1]"
key[CUp]="$terminfo[cuu1]"
key[CDown]="$terminfo[cud1]"
key[CLeft]="$terminfo[cub1]"
key[CRight]="$terminfo[cuf1]"
key[PageUp]="$terminfo[kpp]"
key[PageDown]="$terminfo[knp]"

if [[ -f ${ZDOTDIR:-$HOME}/.zkbd/$TERM-$VENDOR-$OSTYPE ]]; then
  source ${ZDOTDIR:-$HOME}/.zkbd/$TERM-$VENDOR-$OSTYPE
else
  echo "Warning: no zkbd mapping found for $TERM-$VENDOR-$OSTYPE, using terminfo"
fi

. ~/.nix-profile/etc/profile.d/nix.sh
