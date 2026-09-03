#! /usr/bin/env python3

import os.path
import subprocess
import sys


DESIRABLE_LENGTH = 80
ACCEPTABLE_LENGTH = 100


def get_git_command(since):
    return [
        "git",
        "log",
        f"--since={since}",
        "--pretty=format:@@ENTRY@@ %s%n%n%b%n",
    ]


def usage_error(msg):
    print(msg, file=sys.stderr)
    sys.exit(2)


def warn(msg):
    print("WARNING", msg, file=sys.stderr)


def log_skipped(entry):
    print("SKIPPED", entry, file=sys.stderr)


def keep_line(line):
    if line == "---------":
        return False
    if line.startswith("Co-authored-by: "):
        return False
    return True


def keep_entry(entry):
    if entry and entry[0].startswith("- [trivial]"):
        log_skipped(entry)
        return False
    else:
        return True


def check_line_length(line):
    if len(line) <= DESIRABLE_LENGTH:
        pass
    elif len(line) <= ACCEPTABLE_LENGTH:
        warn(f"line long, but acceptable: length {len(line)}")
    else:
        warn(f"line too long: length {len(line)}, {line}")


def format_entry(lines):
    result = []
    is_first = True
    for line in lines:
        if keep_line(line):
            check_line_length(line)
            if line and not is_first:
                line = "    " + line
            result.append(line)
        else:
            warn(f"filtered line: {repr(line)}")
        is_first = False
    while result and not result[-1]:
        result.pop()
    return result


def split_into_entries(stream):
    entry = []
    for line in stream:
        if line.startswith("@@ENTRY@@"):
            line = line.replace("@@ENTRY@@", "-")
            if entry:
                yield format_entry(entry)
                entry = []
        entry.append(line)
    if entry:
        yield format_entry(entry)


def get_command_output_lines(command):
    return subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=True).stdout.splitlines()


def get_since_date_from_command_line():
    prog = os.path.basename(sys.argv[0])
    args = sys.argv[1:]
    if len(args) != 1:
        usage_error(f"usage: {prog} SINCE_DATE\n\n" +
                    f"example: {prog} 2024-10-01")
    return args[0]


def main(**kwargs):
    since = get_since_date_from_command_line()
    command = get_git_command(since)
    log = get_command_output_lines(command)
    for entry in split_into_entries(log):
        if keep_entry(entry):
            for line in entry:
                print(line)
            print()


if __name__ == "__main__":
    """Filter and format the change log entries for a release from the
    commit history.

    Usage:
    ./format_change_log.py START_DATE

    Major functionality:
    - Skip "---------" and "Co-authored-by:" lines that often result from
      github's merge process.
    - Skip change log entries beginning with "[trivial]".
    - Warn about overly long lines. These should then be edited manually.

    The formatted entries are printed to stdout and warnings to
    stderr, so redirection is needed to separate them. It is important
    to check the warnings.

    Recommended usage example:

    SINCE=2024-10-11
    ./format_change_log.py $SINCE > format_change_log.out 2> format_change_log.err
    # Check stderr for suspicious entries:
    sort format_change_log.err | less
    # Edit format_change_log.out manually to deal with "line too long" warnings.
    """
    main()
