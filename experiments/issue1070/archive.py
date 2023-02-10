from pathlib import Path
import subprocess
import tarfile
from tempfile import TemporaryDirectory

ARCHIVE_HOST = "aifiles"
ARCHIVE_LOCATION = Path("experiments")

def add_archive_step(exp, path):
    """
    Adds a step to the given experiment that will archive it to the
    archive location specified in ARCHIVE_LOCATION und the given path.
    We archive the following files:
    - everything in the same directory as the main experiment script
      (except for 'data', '.venv', and '__pycache__')
    - all generated reports
    - the combined properties file
    - all run and error logs
    - the source code stored in the experiment data directory
    - any files added as resources to the experiment

    The first two items in the above list will be stored unpacked for easier
    access while all otherdata will be packed.
    """
    def archive():
        archive_path = ARCHIVE_LOCATION / path
        _archive_script_dir(exp, ARCHIVE_HOST, archive_path)
        _archive_data_dir(exp, ARCHIVE_HOST, archive_path)
        _archive_eval_dir(exp, ARCHIVE_HOST, archive_path)

    exp.add_step("archive", archive)


def _archive_script_dir(exp, host, archive_path):
    """
    Archives everything except 'data', '.venv', and '__pycache__' from the
    same directory as the experiment script at host:archive_path/scripts.
    """
    script_dir = Path(exp._script).parent
    target_path = archive_path / "scripts"

    script_files = [f for f in script_dir.glob("*")
                        if not f.name not in ["data", ".venv", "__pycache__"]]
    _rsync(script_files, host, target_path)


def _archive_data_dir(exp, host, archive_path):
    """
    Packs all files we want to archive from the experiment's data directory and
    then archives the packed data at host:archive_path/data. Specifically, the
    archived files are:
      - all files directly in the data dir (added resources such as parsers)
      - all directories starting with "code_" (source code of all revisions and
        the compilied binaries)
      - All *.log and *.err files from the run directories
    """
    data_dir = Path(exp.path)
    target_path = archive_path / "data"

    data_files = [f for f in data_dir.glob("*") if f.is_file()]
    data_files.extend([d for d in data_dir.glob("code-*") if d.is_dir()])
    data_files.extend(data_dir.glob("runs*/*/*.log"))
    data_files.extend(data_dir.glob("runs*/*/*.err"))
    with TemporaryDirectory() as tmpdirname:
        packed_filename = Path(tmpdirname) / (exp.name + ".tar.xz")
        _pack(data_files, packed_filename, Path(exp.path).parent)
        _rsync([packed_filename], host, target_path)


def _archive_eval_dir(exp, host, archive_path):
    """
    Archives all files in the experiment's eval dir.
    If there is a properties file, it will be packed and only the
    packed version will be included in the resulting list.
    """
    eval_dir = Path(exp.eval_dir)
    target_path = archive_path / "data" / eval_dir.name

    filenames = list(eval_dir.glob("*"))
    properties = eval_dir / "properties"
    if properties.exists():
        filenames.remove(properties)
        with TemporaryDirectory() as tmpdirname:
            packed_properties = Path(tmpdirname) / "properties.tar.xz"
            _pack([properties], packed_properties, eval_dir)
            _rsync([packed_properties], host, target_path)
    _rsync(filenames, host, target_path)


def _pack(filenames, archive_filename, path_prefix):
    """
    Packs all files given in filenames into an archive (.tar.xz) located at
    archive_filename. The path_prefix is removed in the archive, i.e., 
    if the filename is '/path/to/file' and the prefix is '/path', the location
    inside the archive will be 'to/file'.
    """
    with tarfile.open(archive_filename, "w|xz") as f:
        for name in filenames:
            f.add(name, name.relative_to(path_prefix))

def _rsync(filenames, host, target_path):
    # Before copying files we have to create the target path on host.
    # We could use the rsync option --mkpath but it is only available in newer
    # rsync versions (and not in the one running on the grid)
    # https://stackoverflow.com/questions/1636889
    subprocess.run(["ssh", host, "mkdir", "-p", target_path])
    subprocess.run(["rsync", "-avz"] + [str(f) for f in filenames] + [f"{host}:{target_path}"])
