
# Convenience function to reload the rc
alias rcreload="source \"$ZDOTDIR/.zshenv\" && source \"$ZDOTDIR/.zshrc\""

# Enable G1 line drawing character set if stdin isatty
if [[ -t 0 ]]; then
  printf '\x1B)0'
  nixflake --random
fi

# Make run-help/alt-H look in builtin help before man pages
unalias run-help 2>/dev/null
autoload run-help

alias help=run-help
alias cm=chezmoi
alias ls='eza --icons=never --group-directories-first --color=auto'
alias ll='ls -lAmb'
alias la='ls -A'
alias lr='ls -lAmbr -s modifier'

alias grep='grep --color=auto'
alias fgrep='grep -F'
alias egrep='grep -E'
alias rsync='rsync -Ph -ASHAX'

# Man disambiguator when the same page exists in multiple sections.
function man {
  if [[ $2 ]] || (( $(2>/dev/null whatis $1 | wc -l) <= 1 )); then
    # Either the user has specified more than just the man page (section
    # number, extra flags...) or the man page they asked for only has one
    # version anyways.
    command man "$@"; return
  fi

  echo "Which man page do you want? Enter the section number."
  whatis $1 | sort | sed 's,^,  ,'
  read 'section?Man: ' || return
  command man $section $@
}

# Put key bindings in emacs mode.
bindkey -e

# ctrl-left/right arrow to move by words instead of characters
# Yaft doesn't distinguish between normal and ctrl-held arrow keys :(
# bindkey '^[[1;5D' emacs-backward-word
# bindkey '^[[1;5C' emacs-forward-word
bindkey "$key[Home]" emacs-backward-word
bindkey "$key[End]" emacs-backward-word

# home/end/delete support
bindkey "$key[PageUp]" beginning-of-line
bindkey "$key[PageDown]" end-of-line
bindkey "$key[Delete]" delete-char

# local and global history
bindkey "$key[KUp]" up-line-or-local-history
bindkey "$key[KDown]" down-line-or-local-history
bindkey "$key[CUp]" up-line-or-local-history
bindkey "$key[CDown]" down-line-or-local-history

up-line-or-local-history() {
    zle set-local-history 1
    zle up-line-or-history
    zle set-local-history 0
}
zle -N up-line-or-local-history
down-line-or-local-history() {
    zle set-local-history 1
    zle down-line-or-history
    zle set-local-history 0
}
zle -N down-line-or-local-history

histopts=(
  # Don't save duplicates, don't show duplicates in history search.
  hist_expire_dups_first hist_ignore_all_dups hist_find_no_dups hist_save_no_dups
  # Don't save commands prefixed with ' ' or the `history` or `fc` commands
  hist_ignore_space hist_no_store
  # Load command into the edit buffer when doing history expandion
  hist_verify
  # Share history across shell instances; equivalent to turning on
  # inc_append_history and then reloading history each time the prompt is shown.
  share_history
)
setopt ${histopts[@]}

opts=(
  # directories
  no_auto_cd no_auto_pushd pushd_ignore_dups no_pushd_silent no_pushd_to_home
  # completion
  auto_menu no_auto_remove_slash list_ambiguous menu_complete no_rec_exact
  ## expansion
  no_extended_glob no_match
  # history
  no_bang_hist hist_verify no_prompt_bang
  hist_expire_dups_first hist_ignore_all_dups hist_find_no_dups hist_save_no_dups
  hist_ignore_space hist_no_store share_history
  # io & parsing
  interactive_comments rc_quotes
  # job control
  no_bg_nice
  # require '-e' to enable escape processing in echo
  bsd_echo
)
setopt ${=opts[@]}
unset opts histopts
