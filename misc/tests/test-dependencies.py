import os
import subprocess

DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(DIR))
DOWNWARD_FILES = os.path.join(REPO, "src", "search", "DownwardFiles.cmake")
TEST_BUILD_CONFIGS = os.path.join(REPO, "test_build_configs.py")
BUILD = os.path.join(REPO, "build.py")

with open(DOWNWARD_FILES) as d:
    content = d.readlines()

content = [line for line in content if '#' not in line]
content = [line for line in content if 'NAME' in line or 'CORE_PLUGIN' in line or 'DEPENDENCY_ONLY' in line]

plugins_to_be_tested = []
for i in range(0, len(content)):
    if 'NAME' in content[i]:
        if i == len(content)-1 or ('CORE_PLUGIN' not in content[i+1] and 'DEPENDENCY_ONLY' not in content[i+1]):
            plugins_to_be_tested.append(content[i].replace("NAME", "").strip())

with open(TEST_BUILD_CONFIGS, "w+") as f:
    for plugin in plugins_to_be_tested:
        lowercase = plugin.lower()
        line = "%(lowercase)s = [\"-DCMAKE_BUILD_TYPE=Debug\", \"-DDISABLE_PLUGINS_BY_DEFAULT=YES\"," \
               " \"-DPLUGIN_%(plugin)s_ENABLED=True\"]\n" % locals()
        f.write(line)

plugins_failed_test = []
for plugin in plugins_to_be_tested:
    try:
        subprocess.check_call([BUILD, "-j4", plugin.lower()])
    except subprocess.CalledProcessError:
        plugins_failed_test.append(plugin)

if not plugins_failed_test:
    print("\nAll plugins have passed dependencies test.")
else:
    print("\nFailure:\n")
    for plugin in plugins_failed_test:
        print("%(plugin)s failed dependencies test.\n" % locals())
