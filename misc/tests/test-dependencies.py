#! /usr/bin/env python3

import os
import re
import shutil
import subprocess
import sys

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
LIBRARY_DEFINITION_FILE = os.path.join(REPO, "src", "search", "CMakeLists.txt")
BUILDS = os.path.join(REPO, "builds/test-dependencies")


# Find the closing bracket matching the opening bracket that occurred before start
def find_corresponding_closing_bracket(content, start):
    nested_level = 1
    for index, character in enumerate(content[start:]):
        if character == "(":
            nested_level += 1
        if character == ")":
            nested_level -= 1
            if nested_level == 0:
                return index+start

# Extract all "create_fast_downward_library(...)" blocks and if the library is
# not a core or depencency library, add its name to the return list.
def get_library_definitions(content):
    libraries = []
    library_definition_string = r"create_fast_downward_library\("
    pattern = re.compile(library_definition_string)
    for library_match in pattern.finditer(content):
        start = library_match.start()+len(library_definition_string)
        end = find_corresponding_closing_bracket(content, start)
        library_definition = content[start:end]
        # we cannot manually en-/disable core and dependency only libraries
        if any(s in library_definition for s in ["CORE_LIBRARY", "DEPENDENCY_ONLY"]):
            continue
        name_match = re.search(r"NAME\s+(\S+)", library_definition)
        assert name_match
        name = name_match.group(1)
        libraries.append(name)
    return libraries


# Try to get the number of CPUs
try:
    # Number of usable CPUs (Unix only)
    NUM_CPUS = len(os.sched_getaffinity(0))
except AttributeError:
    # Number of available CPUs as a fall-back (may be None)
    NUM_CPUS = os.cpu_count()

# Read in the file where libraries are defined and extract the library names.
with open(LIBRARY_DEFINITION_FILE) as d:
    content = d.read()
content = re.sub(r"#(.*)", "", content) # Remove all comments
libraries = get_library_definitions(content)

# Build each library and add it to libraries_failed_test if not successfull.
libraries_failed_test = []
for library in libraries:
    build_path = os.path.join(BUILDS, library.lower())
    config_cmd = [
        "cmake", "-S", os.path.join(REPO, "src"), "-B", build_path,
        "-DCMAKE_BUILD_TYPE=Debug", "-DDISABLE_LIBRARIES_BY_DEFAULT=YES",
        f"-DLIBRARY_{library.upper()}_ENABLED=True"
    ]
    build_cmd = ["cmake", "--build", build_path]
    if NUM_CPUS:
        build_cmd += ["-j", f"{NUM_CPUS}"]

    try:
        subprocess.check_call(config_cmd)
        subprocess.check_call(build_cmd)
    except subprocess.CalledProcessError:
        libraries_failed_test.append(library)

# Report the result of the test.
if libraries_failed_test:
    print("\nFailure:")
    for library in libraries_failed_test:
        print("{library} failed dependencies test".format(**locals()))
    sys.exit(1)
else:
    print("\nAll libraries have passed dependencies test")
