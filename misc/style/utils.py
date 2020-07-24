import os
import os.path


def get_src_files(path, extensions, ignore_dirs=None):
    ignore_dirs = ignore_dirs or []
    src_files = []
    for root, dirs, files in os.walk(path):
        for ignore_dir in ignore_dirs:
            if ignore_dir in dirs:
                dirs.remove(ignore_dir)
        src_files.extend([
            os.path.join(root, file)
            for file in files if file.endswith(extensions)])
    return src_files
