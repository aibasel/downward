from mercurial import cmdutil
from mercurial import util
try:
    # Mercurial >= 1.8
    # Due to mercurial's lazy importing we need to import the match function
    # directly to raise an ImportError if the scmutil module is not present.
    from mercurial.scmutil import match
    def match_func(repo, ctx, patterns, options):
        return match(ctx, patterns, options)
except ImportError:
    def match_func(repo, ctx, patterns, options):
        return cmdutil.match(repo, patterns, options)

import errno
import os
import os.path
import subprocess

SUFFIX = ".uncrustify.temp"


def _call_subprocesses(cmddesc, *cmd_lists):
    for cmd_list in cmd_lists:
        try:
            exitcode = subprocess.call(cmd_list)
        except OSError as e:
            # If command is not found, swallow error and try next command.
            if e.errno != errno.ENOENT:
                raise util.Abort("error running %s: %s" % (cmddesc, e))
        else:
            return exitcode
    raise util.Abort("could not find %s -- not installed?" % cmddesc)


def _run_uncrustify(config_file, filenames):
    """Call uncrustify, with some fancy additional behaviour:
    1. Make sure that we don't call with an empty file list since
       then uncrustify would wait for input on stdin.
    2. Don't call on empty files since uncrustify doesn't like them.
       Instead, manually create empty outputs for empty inputs.
    3. Barf if the output files for uncrustify already exist.
    """
    inputs = []
    for filename in filenames:
        temp_file = filename + SUFFIX
        if os.path.exists(temp_file):
            raise util.Abort("cannot run uncrustify: %s exists" % temp_file)
        if os.stat(filename).st_size:
            inputs.append(filename)
        else:
            # Create empty fake output file.
            with open(temp_file, "w"):
                pass
    if inputs:
        cmd = ["uncrustify", "-q", "-c", config_file,
               "--suffix", SUFFIX] + inputs
        returncode = _call_subprocesses("uncrustify", cmd)
        if returncode:
            uncrustify_version = subprocess.check_output(
                ["uncrustify", "--version"]).strip()
            raise util.Abort(
                "uncrustify exited with {returncode}. Are you using an "
                "outdated version? We require uncrustify 0.61, you "
                "have {uncrustify_version}. Please consult "
                "www.fast-downward.org/ForDevelopers/Uncrustify".format(
                **locals()))


def _run_diff(oldfile, newfile):
    """Run colordiff if it can be found, and plain diff otherwise."""
    # TODO: It may be nicer to use the internal diff engine for this.
    #       For one, this would use the correct colors set up for hg
    #       diff rather than the colors set up for colordiff. It's not
    #       clear to me how this can be done though, and if it is
    #       worth the bother.
    _call_subprocesses("diff or colordiff",
                       ["colordiff", "-u", oldfile, newfile],
                       ["diff", "-u", oldfile, newfile])


def _get_files(repo, patterns, options):
    """Return all files in the working directory that match the
    patterns and are tracked (clean, modified or added). Ignored or
    unknown files are only matched when given literally.

    If patterns is empty, match all tracked files.

    Supports options['include'] and options['exclude'] which work like
    the --include and --exclude options of hg status.
    """
    ctx = repo[None]
    match = match_func(repo, ctx, patterns, options)
    try:
        status = ctx.status(listclean=True, listignored=True, listunknown=True)
    except TypeError:
        # Compatibility with older Mercurial versions.
        status = ctx.status(clean=True, ignored=True, unknown=True)
    modified = status[0]
    added = status[1]
    unknown = status[4]
    ignored = status[5]
    clean = status[6]
    files = []
    for file_list in [clean, modified, added]:
        for filename in file_list:
            if match(filename):
                files.append(filename)
    for file_list in [ignored, unknown]:
        for filename in file_list:
            if match.exact(filename):
                files.append(filename)
    return files


def uncrustify(ui, repo, *patterns, **options):
    """Run uncrustify on the specified files or directories.

    If no files are specified, operates on the whole working
    directory.

    Note: Files that don't have a .cc or .h suffix are always ignored,
    even if specified on the command line explicitly.

    By default, prints a list of files that are not clean according to
    uncrustify, using a similar output format as with hg status. No
    changes are made to the working directory.

    With the --diff option, prints the changes suggested by uncrustify
    in unified diff format. No changes are made to the working
    directory.

    With the --modify option, actually performs the changes suggested
    by uncrustify. The original (dirty) files are backed up with a
    .crusty suffix. Existing files with such a suffix are silently
    overwritten. To disable these backups, use --no-backup.

    This command always operates on the working directory, not on
    arbitrary repository revisions.

    Returns 0 on success.
    """
    if options["diff"] and options["modify"]:
        raise util.Abort("cannot specify --diff and --modify at the same time")

    if options["diff"]:
        mode = "diff"
    elif options["modify"]:
        mode = "modify"
    else:
        mode = "status"

    no_backup = options["no_backup"]
    show_clean = options["show_clean"]

    paths = [path for path in _get_files(repo, patterns, options)
             if path.endswith((".cc", ".h"))]


    uncrustify_cfg = repo.pathto(".uncrustify.cfg")
    relpaths = [repo.pathto(path) for path in paths]
    if not os.path.exists(uncrustify_cfg):
        raise util.Abort("could not find .uncrustify.cfg in repository root")
    _run_uncrustify(uncrustify_cfg, relpaths)

    ctx = repo[None]
    for path in paths:
        relpath = repo.pathto(path)
        uncr_path = path + SUFFIX
        uncr_relpath = relpath + SUFFIX
        have_changes = (ctx[path].data() != ctx[uncr_path].data())

        if have_changes:
            if mode == "status":
                ui.write("M %s\n" % relpath, label="status.modified")
                util.unlink(uncr_relpath)
            elif mode == "diff":
                _run_diff(relpath, uncr_relpath)
                util.unlink(uncr_relpath)
            elif mode == "modify":
                if not no_backup:
                    util.rename(relpath, relpath + ".crusty")
                util.rename(uncr_relpath, relpath)
                if not ui.quiet:
                    ui.write("%s uncrustified\n" % relpath)
        else:
            if show_clean:
                if mode == "status":
                    ui.write("C %s\n" % relpath, label="status.clean")
                elif mode == "modify":
                    ui.write("%s is clean\n" % relpath)
            util.unlink(uncr_relpath)


cmdtable = {
    "uncrustify": (
        uncrustify,
        [("d", "diff", None,
          "show diff of changes that --modify would make"),
         ("m", "modify", None,
          "actually modify the files that need to be uncrustified"),
         ("", "no-backup", None,
          "do not save backup copies of modified files"),
         ("", "show-clean", None,
          "also list clean source files"),
         ("I", "include", [],
          "include names matching the given patterns", "PATTERN"),
         ("X", "exclude", [],
          "exclude names matching the given patterns", "PATTERN"),
         ], "[OPTION]... [FILE]...")
}
