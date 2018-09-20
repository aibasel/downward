#! /usr/bin/env python

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
content = [line for line in content if 'NAME' in line or 'CORE_PLUGIN' in line or 'DEPENDENCY_ONLY' in line]

plugins_to_be_tested = []
for line in content:
    if 'NAME' in line:
        plugins_to_be_tested.append(line.replace("NAME", "").strip())
    if 'CORE_PLUGIN' in line or 'DEPENDENCY_ONLY' in line:
        plugins_to_be_tested.pop()

with open(TEST_BUILD_CONFIGS, "w") as f:
    for plugin in plugins_to_be_tested:
        lowercase = plugin.lower()
        line = "{lowercase} = [\"-DCMAKE_BUILD_TYPE=Debug\", \"-DDISABLE_PLUGINS_BY_DEFAULT=YES\"," \
               " \"-DPLUGIN_{plugin}_ENABLED=True\"]\n".format(**locals())
        f.write(line)
        paths_to_clean.append(os.path.join(BUILDS, lowercase))

plugins_failed_test = []
for plugin in plugins_to_be_tested:
    try:
        subprocess.check_call([BUILD, plugin.lower()])
    except subprocess.CalledProcessError:
        plugins_failed_test.append(plugin)

if plugins_failed_test:
    print("\nFailure:")
    for plugin in plugins_failed_test:
        print("{plugin} failed dependencies test".format(**locals()))
    sys.exit(1)
else:
    print("\nAll plugins have passed dependencies test")
    clean_up(paths_to_clean)
