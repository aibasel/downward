#! /usr/bin/env python3


import argparse
import re
import subprocess
import sys

CONTRUCTOR_REGEX = r"\b(\w+)(?:\s*::\s*\1\b)+"
CREATE_COMPONENT_REGEX = r"(^|\s|\W)create_component"
C_VAR_PATTERN = r'[^a-zA-Z0-9_]' # overapproximation
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


def check_concrete_constructor2(cc_file, class_name, naive=False):
    with open(cc_file, 'r') as file:
        content = file.read()
    pattern = rf'{class_name}\s*\((.*?)\)'
    match = re.search(pattern, content, re.DOTALL)

    if match:
        parameters = match.group(1).strip()
        return (True, parameters)
    else:
        return (False, "")

def extract_balanced_parentheses(s, feature_name):

    position = s.find(feature_name + "(")

    if position != -1:
        s = s[position:]
    else:
        print("'" + feature_name + "(' not found in the string.")

    s = s.split(')\\n-')[0]

    return s[len(feature_name)+1::]

def remove_balanced_brackets(s):
    stack = []
    result = []
    for c in s:
        if c == '[':
            stack.append(c)
        elif c == ']':
            if stack and stack[-1] == '[':
                stack.pop()
            else:
                result.append(c)
        elif not stack:
            result.append(c)
    return ''.join(result)

def extract_feature_paras(s, feature_name):
    s = str(s)

    in_parenthesis = extract_balanced_parentheses(s, feature_name)

    in_parenthesis_2 = remove_balanced_brackets(in_parenthesis)
    result = re.sub(r'=.*?,', ',', in_parenthesis_2 + ",").split()

    return result


def extract_feature_name_and_class(cc_file, args):
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    feature_name = []
    feature_class = []
    name_pattern = r'TypedFeature\("([^"]*)"\)'
    class_pattern = r'TypedFeature<(.*?)>'
    for line in source_without_comments.splitlines():
        if re.search(name_pattern, line):
            feature_name = re.search(name_pattern, line).group(1)
        if re.search(class_pattern, line):
            feature_class = re.search(class_pattern, line).group(1)
    class_name = feature_class.split()[-1].split("::")[-1]
    other_namespace = len(feature_class.split()[-1].split("::")) == 2
    return feature_name, class_name, other_namespace

def get_feature_paras(feature_name):
    result = subprocess.run(["./../../builds/release/bin/downward", "--help", "--txt2tags", "{}".format(feature_name)], stdout=subprocess.PIPE)

    feature_paras = extract_feature_paras(result.stdout, feature_name)
    return feature_paras

def get_class_paras(class_name, other_namespace, cc_file, args):
    b, s = check_concrete_constructor2(cc_file, class_name)
    if not b:
        for cc_file2 in args.cc_file:
            b, s = check_concrete_constructor2(cc_file2, class_name, naive=True)
            if b:
                break

    if b:
        s = s.replace("\n", "") + ","
        s = s.split()
        s = [word for word in s if "," in word]
        s = [re.sub(C_VAR_PATTERN, '', word) + "," for word in s]
        return s
    else:
        print("<<<<<<<<<<<not found: " + class_name)
        return [","]

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
    found_error = False
    error = ""
    component_creations = check_create_component(cc_file)
    if not component_creations == []:
        for component_creation in component_creations:
            component_class = extract_component_class(component_creation)
            error += ". . .\n"
            error += ". .\n"
            error += ".\n"
            error += "\n"
            error += ".\n"
            error += ". .\n"
            error += ". . .\n"
            error += "= = = " + component_class + " = = =\n"
            feature_name, class_name, other_namespace = extract_feature_name_and_class(cc_file, args)
            feature_paras = get_feature_paras(feature_name)

            error += "== FEATURE PARAS '" + feature_name + "'==\n"
            error += str(feature_paras) + "\n"

            class_paras = get_class_paras(class_name, other_namespace, cc_file, args)

            error += "== CLASS PARAS '" + class_name + "'==\n"
            error += str(class_paras) + "\n"

            if feature_paras != class_paras:
                found_error = True
                if not len(feature_paras) == len(class_paras):
                    error += "Wrong sizes\n"
                for i in range(min(len(feature_paras), len(class_paras))):
                    if feature_paras[i] != class_paras[i]:
                        error += feature_paras[i] + " =/= " + class_paras[i] + "\n"
                error += cc_file + "\n"

    return found_error, error,

def main():
    args = parse_args()
    errors = []
    for cc_file in args.cc_file:
        found_error, error = print_component_paras(cc_file, args)
        if found_error:
            errors.append(error)
    print("#########################################################")
    print("#########################################################")
    print("#########################################################")
    print("#########################################################")
    print("#########################################################")
    print(".: ERRORS :.")
    for error in errors:
        print(error)
    if errors:
        sys.exit(1)


if __name__ == "__main__":
    main()
