#! /usr/bin/env python3

import os
import shutil
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
DOWNWARD_FILES = os.path.join(REPO, "src", "search", "DownwardFiles.cmake")
TEST_BUILD_CONFIGS = os.path.join(REPO, "test_build_configs.py")
BUILD = os.path.join(REPO, "build.py")
BUILDS = os.path.join(REPO, "builds")
paths_to_clean = [TEST_BUILD_CONFIGS]


def clean_up(paths_to_clean):
    print("\nCleaning up")
    for path in paths_to_clean:
        print("Removing {path}".format(**locals()))
        if os.path.isfile(path):
            os.remove(path)
        if os.path.isdir(path):
            shutil.rmtree(path)
    print("Done cleaning")


with open(DOWNWARD_FILES) as d:
    content = d.readlines()

content = [line for line in content if '#' not in line]
content = [line for line in content if 'NAME' in line or 'CORE_LIBRARY' in line or 'DEPENDENCY_ONLY' in line]

libraries_to_be_tested = []
for line in content:
    if 'NAME' in line:
        libraries_to_be_tested.append(line.replace("NAME", "").strip())
    if 'CORE_LIBRARY' in line or 'DEPENDENCY_ONLY' in line:
        libraries_to_be_tested.pop()

with open(TEST_BUILD_CONFIGS, "w") as f:
    for library in libraries_to_be_tested:
        lowercase = library.lower()
        line = "{lowercase} = [\"-DCMAKE_BUILD_TYPE=Debug\", \"-DDISABLE_LIBRARIES_BY_DEFAULT=YES\"," \
               " \"-DLIBRARY_{library}_ENABLED=True\"]\n".format(**locals())
        f.write(line)
        paths_to_clean.append(os.path.join(BUILDS, lowercase))

libraries_failed_test = []
for library in libraries_to_be_tested:
    try:
        subprocess.check_call([BUILD, library.lower()])
    except subprocess.CalledProcessError:
        libraries_failed_test.append(library)

if libraries_failed_test:
    print("\nFailure:")
    for library in libraries_failed_test:
        print("{library} failed dependencies test".format(**locals()))
    sys.exit(1)
else:
    print("\nAll libraries have passed dependencies test")
    clean_up(paths_to_clean)
