## Tab completion for Fast Downward

We support tab completion for bash and zsh based on the python package [argcomplete](https://pypi.org/project/argcomplete/). For full support, use at least version 3.3 which can be installed via `pip`.

```bash
pip install argcomplete>=3.3
```

After the installation, add the following commands to your `.bashrc` or `.zshrc`. Depending on your installation replace `register-python-argcomplete` with `register-python-argcomplete3`.

```bash
eval "$(register-python-argcomplete fast-downward.py)"
eval "$(register-python-argcomplete build.py)"
eval "$(register-python-argcomplete translate.py)"

function _downward_complete() {
    local IFS=$'\013'
    if [[ -n "${ZSH_VERSION-}" ]]; then
        local DFS=":"
        local completions
        local COMP_CWORD=$((CURRENT - 1))
        completions=( $( "${words[1]}" --bash-complete \
                         "$IFS" "$DFS" "$CURSOR" "$BUFFER" "$COMP_CWORD" ${words[@]}))
        if [[ $? == 0 ]]; then
            _describe "${words[1]}" completions -o nosort
        fi
    else
        local DFS=""
        COMPREPLY=( $( "$1" --bash-complete \
                    "$IFS" "$DFS" "$COMP_POINT" "$COMP_LINE" "$COMP_CWORD" ${COMP_WORDS[@]}))
        if [[ $? != 0 ]]; then
            unset COMPREPLY
        fi
    fi
}

if [[ -n "${ZSH_VERSION-}" ]]; then
    compdef _downward_complete downward
else
    complete -o nosort -F _downward_complete downward
fi
```

Restart your shell afterwards.
