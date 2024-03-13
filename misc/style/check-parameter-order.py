#! /usr/bin/env python3


import argparse
import re
import subprocess
import sys

SHORT_HANDS = {
                "ipdb": "cpdbs(hillclimbing())",
                "astar": "eager(tiebreaking([sum([g(), h]), h], unsafe_pruning=false), reopen_closed=true, f_eval=sum([g(), h]))",
                "lazy_greedy": "lazy(alt([single(h1), single(h1, pref_only=true), single(h2), single(h2, pref_only=true)], boost=100), preferred=h2)",
                "lazy_wastar": "lazy(single(sum([g(), weight(eval1, 2)])), reopen_closed=true)",
                "eager_greedy": "eager(single(eval1))",
                "eager_wastar": """ See corresponding notes for "(Weighted) A* search (lazy)" """,


}

TEMPORARY_EXCEPTIONS = [
    "iterated",
    "eager",
    "sample_based_potentials",
    "initial_state_potential",
    "all_states_potential",
    "diverse_potentials",
]

PERMANENT_EXCEPTIONS = [
    "adapt_costs"
]

CREATE_COMPONENT_REGEX = r"(^|\s|\W)create_component"
C_VAR_PATTERN = r'[^a-zA-Z0-9_]' # overapproximation
def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("cc_file", nargs="+")
    return parser.parse_args()

def extract_cpp_class(input_string):
    pattern = r'<(.*?)>'
    match = re.search(pattern, input_string)
    assert match
    return match.group(1)


def get_constructor_parameters(cc_file, class_name):
    with open(cc_file, 'r') as file:
        content = file.read()
    pattern = rf'{class_name}\s*\((.*?)\)'
    match = re.search(pattern, content, re.DOTALL)

    if match:
        parameters = match.group(1).strip()
        return (True, parameters)
    else:
        return (False, "")

def matching(opening, closing):
    return (opening, closing) == ('(', ')') or (opening, closing) == ('[', ']')

def extract_feature_parameter_list(feature_name):
    s = str(subprocess.run(["./../../builds/release/bin/downward", "--help", "--txt2tags", "{}".format(feature_name)], stdout=subprocess.PIPE).stdout)
    position = s.find(feature_name + "(")
    assert position != -1
    s = s[position + len(feature_name) + 1::] # start after the first '('
    stack = ['(']
    result = []
    for c in s:
        if c == '(' or c == "[":
            stack.append(c)
        elif c == ')' or c == "]":
            assert matching(stack[-1], c)
            stack.pop()
            if not stack:
                break
        if len(stack) == 1: # not within nested parenthesis/brackets
            result.append(c)
    result = ''.join(result)
    result = re.sub(r'=.*?,', ',', result + ",").split()
    return result

def extract_feature_name_and_cpp_class(cc_file, args, num):
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")

    name_pattern = r'TypedFeature\("([^"]*)"\)'
    class_pattern = r'TypedFeature<(.*?)>'

    feature_names = []
    class_names = []
    other_namespaces = []
    feature_error_msgs = []
    class_error_msgs = []
    for line in source_without_comments.splitlines():
        if re.search(name_pattern, line):
            feature_name = re.search(name_pattern, line).group(1)
            feature_error_msg = "feature_name: " + feature_name + "\n"
            feature_names.append(feature_name)
            feature_error_msgs.append(feature_error_msg)

        if re.search(class_pattern, line):
            feature_class = re.search(class_pattern, line).group(1)
            class_name = feature_class.split()[-1].split("::")[-1]
            other_namespace = len(feature_class.split()[-1].split("::")) == 2
            class_error_msg = "class_name: " + class_name + "\n"
            class_names.append(class_name)
            other_namespaces.append(other_namespace)
            class_error_msgs.append(class_error_msg)
    return feature_names[num], class_names[num], other_namespaces[num], feature_error_msgs[num] + class_error_msgs[num]

def get_cpp_class_parameters(class_name, other_namespace, cc_file, args):
    found_in_file, parameters = get_constructor_parameters(cc_file, class_name)
    if not found_in_file:
        # check in all files
        for cc_file2 in args.cc_file:
            found_in_file, parameters = get_constructor_parameters(cc_file2, class_name)
            if found_in_file:
                break

    if found_in_file:
        parameters = parameters.replace("\n", "") + ","
        parameters = parameters.split()
        parameters = [word for word in parameters if "," in word]
        parameters = [re.sub(C_VAR_PATTERN, '', word) + "," for word in parameters]
        return parameters
    else:
        # assume default constructor
        return [","]

def get_create_component_lines(cc_file):
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    lines = []
    for line in source_without_comments.splitlines():
        if re.search(CREATE_COMPONENT_REGEX, line):
            lines.append(line.strip())
    return lines

def compare_component_parameters(cc_file, args):
    found_error = False
    error_msg = ""
    create_component_lines = get_create_component_lines(cc_file)
    if not create_component_lines == []:
        for i, create_component_line in enumerate(create_component_lines):
            feature_name, cpp_class, other_namespace, extracted_error_msg = extract_feature_name_and_cpp_class(cc_file, args, i)
            error_msg += "\n\n=====================================\n= = = " + cpp_class + " = = =\n"
            error_msg += extracted_error_msg + "\n"
            feature_parameters = extract_feature_parameter_list(feature_name)

            error_msg += "== FEATURE PARAMETERS '" + feature_name + "'==\n"
            error_msg += str(feature_parameters) + "\n"

            cpp_class_parameters = get_cpp_class_parameters(cpp_class, other_namespace, cc_file, args)

            error_msg += "== CLASS PARAMETERS '" + cpp_class + "'==\n"
            error_msg += str(cpp_class_parameters) + "\n"

            if feature_name in SHORT_HANDS: # TODO set comparison
                print(f"feature_name '{feature_name}' would trigger an error if it was not marked as shorthand")
            elif feature_name in PERMANENT_EXCEPTIONS:
                print(f"feature_name '{feature_name}' would trigger an error if it was not marked as PERMANENT_EXCEPTION")
            elif feature_name in TEMPORARY_EXCEPTIONS:
                print(f"feature_name '{feature_name}' would trigger an error if it was not marked as TEMPORARY_EXCEPTION")
            elif feature_parameters != cpp_class_parameters:
                found_error = True
                if not len(feature_parameters) == len(cpp_class_parameters):
                    error_msg += "Wrong sizes\n"
                for i in range(min(len(feature_parameters), len(cpp_class_parameters))):
                    if feature_parameters[i] != cpp_class_parameters[i]:
                        error_msg += feature_parameters[i] + " =/= " + cpp_class_parameters[i] + "\n"
                error_msg += cc_file + "\n"

    return found_error, error_msg

def main():
    print(str(subprocess.run(["pwd"], stdout=subprocess.PIPE).stdout))
    # print(str(subprocess.run(["tree", "-L", "3"], stdout=subprocess.PIPE).stdout))
    tree = subprocess.run(["tree", "-L", "4"], stdout=subprocess.PIPE, text=True)
    print(tree.stdout)
    args = parse_args()
    errors = []
    for cc_file in args.cc_file:
        found_error, error = compare_component_parameters(cc_file, args)
        if found_error:
            errors.append(error)
    if errors:
        print("#########################################################")
        print("#########################################################")
        print("#########################################################")
        print("#########################################################")
        print("#########################################################")
        print(".: ERRORS :.")
        for error in errors:
            print(error)
        sys.exit(1)


if __name__ == "__main__":
    main()
