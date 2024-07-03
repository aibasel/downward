#! /usr/bin/env python3

HELP = """\
Check that parameters for the command line features match the parameters
 of the C++ constructors. Use txt2tags to compare the parameters
 mentioned in the wiki to the parameters in the corresponding .cc file.
"""

import argparse
from pathlib import Path
import re
import subprocess
import sys

DIR = Path(__file__).resolve().parent
REPO = DIR.parents[1]
SRC_DIR = REPO / "src"

SHORT_HANDS = [
    "ipdb", # cpdbs(hillclimbing())
    "astar", # eager(tiebreaking([sum([g(), h]), h],
            #   unsafe_pruning=false), reopen_closed=true,
            #   f_eval=sum([g(), h]))
    "lazy_greedy", # lazy(alt([single(h1), single(h1,
                  #  pref_only=true), single(h2), single(h2,
                  #  pref_only=true)], boost=100), preferred=h2)
    "lazy_wastar", # lazy(single(sum([g(), weight(eval1, 2)])),
                  # reopen_closed=true)
    "eager_greedy", # eager(single(eval1))
    "eager_wastar", # See corresponding notes for
                   # "(Weighted) A* search (lazy)"
                   # eager_wastar(evals=[eval1, eval2],prefered=pref1,
                   #  reopen_closed=rc1, boost=boo1, w=w1,
                   #  pruning=pru1, cost_type=ct1, bound = bou1,
                   #  max_time=mt1, verbosity=v1)
                   # Is equivalent to:
                   # eager(open = alt([single(sum([g(), weight(eval1, w1)])),
                   #                   single(sum([g(), weight(eval2, w1)]))],
                   #                  boost=boo1),
                   #  reopen_closed=rc1, f_eval = <none>,
                   #  preferred = pref1, pruning = pru1,
                   #  cost_type=ct1, bound=bou1, max_time=mt1,
                   #  verbosity=v1)
]

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
NON_C_VAR_PATTERN = r'[^a-zA-Z0-9_]' # overapproximation

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
    return ((opening, closing) == ('(', ')') or
        (opening, closing) == ('[', ']'))

def extract_feature_parameter_list(feature_name):
    s = str(subprocess.run(["./../../builds/release/bin/downward",
                            "--help", "--txt2tags",
                            "{}".format(feature_name)],
                           stdout=subprocess.PIPE).stdout)
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
    result = [re.sub(',', '', word) for word in result]
    return result

def extract_feature_name_and_cpp_class(cc_file, cc_files, cwd, num):
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    name_pattern = r'TypedFeature\("([^"]*)"\)'
    class_pattern = r'TypedFeature<(.*?)> {'
    feature_names = []
    class_names = []
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
            class_error_msg = "class_name: " + class_name + "\n"
            class_names.append(class_name)
            class_error_msgs.append(class_error_msg)
    return (feature_names[num], class_names[num],
           feature_error_msgs[num] + class_error_msgs[num])

def get_cpp_class_parameters(
        class_name, cc_file, cc_files, cwd):
    found_in_file, parameters = get_constructor_parameters(
        cc_file, class_name)
    if not found_in_file:
        # check in all files
        for cc_file2 in cc_files:
            found_in_file, parameters = get_constructor_parameters(
                cc_file2, class_name)
            if found_in_file:
                break
    if found_in_file:
        parameters = parameters.replace("\n", "") + ","
        parameters = parameters.split()
        parameters = [word for word in parameters if "," in word]
        parameters = [re.sub(NON_C_VAR_PATTERN, '', word)
                      for word in parameters]
        return parameters
    else:
        # assume default constructor
        return ['']

def get_create_component_lines(cc_file):
    source_without_comments = subprocess.check_output(
        ["gcc", "-fpreprocessed", "-dD", "-E", cc_file]).decode("utf-8")
    lines = []
    for line in source_without_comments.splitlines():
        if re.search(CREATE_COMPONENT_REGEX, line):
            lines.append(line.strip())
    return lines

def compare_component_parameters(cc_file, cc_files, cwd):
    error_msg = ""
    create_component_lines = get_create_component_lines(cc_file)
    if create_component_lines:
        for i, create_component_line in enumerate(
                create_component_lines):
            (feature_name, cpp_class, extracted_error_msg) = (
                extract_feature_name_and_cpp_class(
                    cc_file, cc_files, cwd, i))
            feature_parameters = extract_feature_parameter_list(
                    feature_name)
            cpp_class_parameters = get_cpp_class_parameters(
                cpp_class, cc_file, cc_files, cwd)
            if feature_name in SHORT_HANDS:
                print(f"feature_name '{feature_name}' is ignored"
                    " because it is marked as shorthand")
            elif feature_name in PERMANENT_EXCEPTIONS:
                print(f"feature_name '{feature_name}' is ignored"
                    " because it is marked as PERMANENT_EXCEPTION")
            elif feature_name in TEMPORARY_EXCEPTIONS:
                print(f"feature_name '{feature_name}' is ignored"
                    " because it is marked as TEMPORARY_EXCEPTION")
            elif feature_parameters != cpp_class_parameters:
                error_msg += ("\n\n=====================================\n"
                    + "= = = " + cpp_class + " = = =\n"
                    + extracted_error_msg + "\n"
                    + "== FEATURE PARAMETERS '"
                    + feature_name + "' ==\n"
                    + str(feature_parameters) + "\n"
                    + "== CLASS PARAMETERS '"
                    + cpp_class + "' ==\n"
                    + str(cpp_class_parameters) + "\n")
                if not len(feature_parameters) == len(cpp_class_parameters):
                    error_msg += "Wrong sizes\n"
                for i in range(min(len(feature_parameters),
                                len(cpp_class_parameters))):
                    if feature_parameters[i] != cpp_class_parameters[i]:
                        error_msg += (feature_parameters[i] +
                            " =/= " + cpp_class_parameters[i] + "\n")
                error_msg += cc_file + "\n"
    return error_msg

def error_check(cc_files, cwd):
    errors = []
    for cc_file in cc_files:
        error_msg = compare_component_parameters(
            cc_file, cc_files, cwd)
        if error_msg:
            errors.append(error_msg)
    if errors:
        print("###############    ERRORS    ##########################")
        for error in errors:
            print(error)
        sys.exit(1)

def main():
    """
    Currently, we only check that the parameters in the Constructor in
    the .cc file matches the parameters for the CLI.
    """
    search_dir = SRC_DIR / "search"
    cc_files = [str(file) for file in search_dir.rglob('*.cc')
                if file.is_file()]
    assert len(cc_files) > 0
    print("Checking Component Parameters of"
          " {} *.cc files".format(len(cc_files)))
    return error_check(cc_files, cwd=DIR) == 0

if __name__ == "__main__":
    main()
