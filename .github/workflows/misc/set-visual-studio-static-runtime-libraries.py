"""
Use this script to set the "RuntimeLibrary" property of a Visual Studio project
file to static linking libraries for all configurations.
"""
from xml.etree import ElementTree
import os
import sys

ElementTree.register_namespace(
    "", "http://schemas.microsoft.com/developer/msbuild/2003")


def adapt(in_project, out_project):
    tree = ElementTree.parse(in_project)

    root = tree.getroot()
    for item_definition_group in root.findall(
            '{http://schemas.microsoft.com/developer/msbuild/2003}ItemDefinitionGroup'):
        condition = item_definition_group.attrib["Condition"]
        is_release = any(x in condition.lower()
                         for x in ["release", "minsizerel", "relwithdebinfo"])
        is_debug = "debug" in condition.lower()
        assert is_release ^ is_debug, condition

        compiler_args = item_definition_group.findall(
            '{http://schemas.microsoft.com/developer/msbuild/2003}ClCompile')
        assert len(compiler_args) == 1, compiler_args
        compiler_args = compiler_args[0]

        runtime_library = compiler_args.findall(
            '{http://schemas.microsoft.com/developer/msbuild/2003}RuntimeLibrary')
        if len(runtime_library) == 0:
            runtime_library = ElementTree.Element("RuntimeLibrary")
            compiler_args.append(runtime_library)
        elif len(runtime_library) == 1:
            runtime_library = runtime_library[0]
        else:
            assert False, runtime_library

        runtime_library.text = "MultiThreaded"
        if is_debug:
            runtime_library.text += "Debug"

    tree.write(out_project)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(f"{sys.argv[0]} [OLD_VS_PROJECT_FILE] [OUTPUT_FILE]")
    _, in_path, out_path = sys.argv
    if not os.path.isfile(in_path):
        sys.exit(f"{in_path} does not exist!")

    adapt(in_path, out_path)
