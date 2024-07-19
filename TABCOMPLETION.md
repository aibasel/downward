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
    local IFS=$'\n'
    COMPREPLY=( $( "$1" --bash-complete \
                   "$COMP_POINT" "$COMP_LINE" "$COMP_CWORD" ${COMP_WORDS[@]}))
}
complete -o nosort -o default -o bashdefault -F _downward_complete downward
```

Restart your shell afterwards.
