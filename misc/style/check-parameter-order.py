#! /usr/bin/env python3


import argparse
import re
import subprocess
import sys

CONTRUCTOR_REGEX = r"\b(\w+)(?:\s*::\s*\1\b)+"
CREATE_COMPONENT_REGEX = r"(^|\s|\W)create_component"

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("cc_file", nargs="+")
    return parser.parse_args()

def check_constructor(cc_file):
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    errors = []
    for line in source_without_comments.splitlines():
        if re.search(CONTRUCTOR_REGEX, line):
            errors.append("Found Constructor from\n {}: {}".format(
                cc_file, line.strip()))
    return errors
    
def extract_component_class(input_string):
    # The regex pattern to find text within < >
    pattern = r'<(.*?)>'
    # Search for the pattern in the input string
    match = re.search(pattern, input_string)
    # If a match is found, return the first group (the content inside < >)
    if match:
        return match.group(1)
    # If no match is found, return None or an empty string
    return None
    
def check_concrete_constructor(cc_file, class_name):
    pattern = r"(^|\s|\W)"
    pattern += class_name+"::"+class_name
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    errors = []
    for line in source_without_comments.splitlines():
        if re.search(pattern, line):
            print("Found check_concrete_constructor from\n {}: {}".format(
                cc_file, line.strip()))

def extract_feature_paras(s, feature_name):
    print("XXXXXXXXXXXXX")
    print(s)
    match = re.search(r'epsilon_greedy\((.*?)\)', s)

    # Extract the substring
    if match:
        extracted_substring = match.group(1)
        print(extracted_substring)
    else:
        print("No match found")

def extract_feature_name_and_class(cc_file, args):
    print("- - - - - - - - - -")
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    feature_name = []
    feature_class = []
    name_pattern = r'TypedFeature\("([^"]*)"\)'
    class_pattern = r'TypedFeature<(.*?)>'
    for line in source_without_comments.splitlines():
        if re.search(name_pattern, line):
            feature_name =  re.search(name_pattern, line).group(1)
        if re.search(class_pattern, line):
            feature_class = re.search(class_pattern, line).group(1)
    print("name: ", feature_name)
    print("class: ", feature_class.split()[-1].split("::")[-1])
    result = subprocess.run(["./../../builds/release/bin/downward", "--help", "--txt2tags", "{}".format(feature_name)], stdout=subprocess.PIPE)
    
    extract_feature_paras(result.stdout, feature_name)
    
    #print(result.stdout)
    print("^^^^^^^^^^^^")
    
    if len(feature_class.split()[-1].split("::"))==2:
        for cc_file2 in args.cc_file:
            check_concrete_constructor(cc_file2, feature_class.split()[-1].split("::")[-1])
    else:
        check_concrete_constructor(cc_file, feature_class.split()[-1].split("::")[-1])
    
def check_create_component(cc_file):
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    errors = []
    for line in source_without_comments.splitlines():
        if re.search(CREATE_COMPONENT_REGEX, line):
            errors.append("Found Create_Component from\n {}: {}".format(
                cc_file, line.strip()))
    return errors

def print_component_paras(cc_file, args):
    component_creations = check_create_component(cc_file)
    if not component_creations == []:
        for component_creation in component_creations:
            component_class = extract_component_class(component_creation)
            print(component_class)
            extract_feature_name_and_class(cc_file, args)

def main():
    args = parse_args()

    errors = []
    for cc_file in args.cc_file:
    	print_component_paras(cc_file, args)
    for error in errors:
        print(error)
    if errors:
        sys.exit(1)


if __name__ == "__main__":
    main()
