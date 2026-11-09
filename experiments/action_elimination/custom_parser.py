import logging
import re

from lab.parser import Parser


class CommonParser(Parser):
    def add_repeated_pattern(
        self, name, regex, file="run.log", required=False, type=int
    ):
        def find_all_occurences(content, props):
            matches = re.findall(regex, content)

            if required and not matches:
                logging.error(f"Pattern {regex} not found in file {file}")

            props[name] = [type(m) for m in matches]

        self.add_function(find_all_occurences, file=file)

    def add_bottom_up_pattern(
        self, name, regex, file="run.log", required=False, type=int
    ):
        def search_from_bottom(content, props):
            reversed_content = "\n".join(
                reversed(content.splitlines())
            )

            match = re.search(regex, reversed_content)

            if required and not match:
                logging.error(
                    f"Pattern {regex} not found in file {file}"
                )

            if match:
                props[name] = type(match.group(1))

        self.add_function(search_from_bottom, file=file)


def parse_anytime_plans(content, props):
    costs = []
    times = []

    current_time = None

    for line in content.splitlines():
        time_match = re.search(r"\[t=([0-9.]+)s,", line)

        if time_match:
            current_time = float(time_match.group(1))

        cost_match = re.search(r"Plan cost: ([0-9]+)", line)

        if cost_match:
            costs.append(int(cost_match.group(1)))
            times.append(current_time)

    if costs:
        props["anytime_plan_costs"] = costs
        props["anytime_plan_times"] = times
        props["first_plan_cost"] = costs[0]
        props["final_plan_cost"] = costs[-1]
        props["num_plans"] = len(costs)

def get_parser():
    parser = CommonParser()

    parser.add_bottom_up_pattern(
        "search_start_time",
        r"[t=(.+)s, \d+ KB] g=0, 1 evaluated, 0 expanded",
        type=float,
    )

    parser.add_bottom_up_pattern(
        "search_start_memory",
        r"[t=.+s, (\d+) KB] g=0, 1 evaluated, 0 expanded",
        type=int,
    )

    parser.add_pattern(
        "initial_h_value",
        r"f = (\d+) [1 evaluated, 0 expanded, t=.+s, \d+ KB]",
        type=int,
    )

    parser.add_repeated_pattern(
        "h_values",
        r"New best heuristic value for .+: (\d+)\n",
        type=int,
    )

    parser.add_pattern(
        "plan_improvement_time",
        r"Plan improvement time: ([0-9.]+)s",
        type=float,
    )
    parser.add_pattern(
        "allowed_operator_name_schemas",
        r"Number of operator schemas allowed in reduced search: (\d+)",
        type=int,
    )
    parser.add_pattern(
        "number_of_reduced_operators",
        r"Number of operators allowed in reduced search: (\d+)",
        type=int,
    )
    parser.add_pattern(
        "number_of_operators",
        r"Number of operators in original task: (\d+)",
        type=int,
    )
    parser.add_pattern(
        "graph_states",
        r"Graph states: (\d+)",
        type=int,
    )
    parser.add_pattern(
        "graph_edges",
        r"Graph edges: (\d+)",
        type=int,
    )
    parser.add_pattern(
        "plan_improvement_time",
        r"Plan improvement time: ([\d.]+)s",
        type=float,
        required=False,
    )
    parser.add_repeated_pattern(
    "plan_improvement_times",
    r"Plan improvement time: ([0-9.]+)s",
    type=float,
)
    parser.add_function(parse_anytime_plans, file="run.log")
    return parser