import os
import subprocess


def execute(command, **kwargs):
    """ A thin wrapper over subprocess.call """
    cwd = kwargs.get("cwd", os.getcwd())
    command = command.split() if isinstance(command, str) else command

    print('Executing "{}" in directory "{}"'.format(' '.join(command), cwd))
    return subprocess.call(command, cwd=cwd)
