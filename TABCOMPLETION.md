## Tab completion for Fast Downward

We support tab completion for bash and zsh based on the python package [argcomplete](https://pypi.org/project/argcomplete/). For full support, use at least version 3.3 which can be installed via `pip`.

```bash
pip install "argcomplete>=3.3"
```

After the installation, tab completion has to be enabled in one of two ways.


### Activating tab completion globally

The global activation will enable tab completion for *all* Python files that support argcomplete (not only files related to Fast Downward). To activate the global tab completion execute the following command. Depending on your installation replace `activate-global-python-argcomplete` with `activate-global-python-argcomplete3`.

```bash
activate-global-python-argcomplete
```


### Activating tab completion locally

In contrast to global activation, local activation only enables tab completion for files called `fast-downward.py`, `build.py`, or `translate.py`. However, activation is not limited to files that support argcomplete. This means that pressing tab on older version of Fast Downward files or unrelated files with the same name may have unintended side effects. For example, with older version of Fast Downward `build.py <TAB>` will start a build without printing the output.

To activate the local tab completion, add the following commands to your `.bashrc` or `.zshrc`. Depending on your installation replace `register-python-argcomplete` with `register-python-argcomplete3`.

```bash
eval "$(register-python-argcomplete fast-downward.py)"
eval "$(register-python-argcomplete build.py)"
eval "$(register-python-argcomplete translate.py)"
```

### Activating tab completion for the search binary

If you are working with the search binary `downward` directly, adding the following commands to your `.bashrc` or `.zshrc` will enable tab completion. This is only necessary if you are not using the driver script `fast-downward.py`.

```bash
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


### Limitations

The search configuration following the `--search` option is not yet covered by tab completion. For example, `fast-downward.py problem.pddl --search "ast<TAB>"` will not suggest `astar`.