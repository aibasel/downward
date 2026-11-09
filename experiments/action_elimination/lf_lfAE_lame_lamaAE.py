#!/usr/bin/env python3

import os

import custom_parser
import project

REPO = project.get_repo_base()
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]

REVISION_CACHE = (
    os.environ.get("DOWNWARD_REVISION_CACHE")
    or project.DIR / "data" / "revision-cache"
)


# ---------------------------------------------------------------------------
# Environment and benchmark suite
# ---------------------------------------------------------------------------

if project.REMOTE:
    SUITE = project.SUITE_SATISFICING

    ENV = project.BaselSlurmEnvironment(
        partition="infai_2",
        memory_per_cpu="9000M",
        cpus_per_task=1,
        time_limit_per_task="00:35:00",
    )

else:
    SUITE = [
        "blocks:probBLOCKS-4-0.pddl",
        "blocks:probBLOCKS-5-0.pddl",
        "blocks:probBLOCKS-10-0.pddl",
        "blocks:probBLOCKS-15-0.pddl",
        "elevators-sat08-strips:p01.pddl",
        "elevators-sat08-strips:p02.pddl",
        "elevators-sat08-strips:p23.pddl"
    ]

    ENV = project.LocalEnvironment(processes=12)


# ---------------------------------------------------------------------------
# Planner configuration
# ---------------------------------------------------------------------------

LAMA_FIRST_SEARCH = (
    "let(hlm,"
        "eval_modify_costs("
            "landmark_sum("
                "lm_factory=lm_reasonable_orders_hps(lm_rhw()),"
                "pref=false"
            "),"
            "cost_type=one"
        "),"
        "let(hff,"
            "eval_modify_costs(ff(),cost_type=one),"
            "lazy_greedy("
                "[hff,hlm],"
                "preferred=[hff,hlm],"
                "cost_type=one,"
                "reopen_closed=false"
            ")"
        ")"
    ")"
)

LF_AE = (
    "plan_improvement("
    f"search={LAMA_FIRST_SEARCH},"
    "method=action_elimination()"
    ")"
)

# Full LAMA uses one-cost heuristics for its first phase
# and plusone-cost heuristics for weighted A*.

HLM_PLUSONE = (
    "eval_modify_costs("
        "landmark_sum("
            "lm_factory=lm_reasonable_orders_hps(lm_rhw()),"
            "pref=false"
        "),"
        "cost_type=plusone"
    ")"
)

HFF_PLUSONE = "eval_modify_costs(ff(),cost_type=plusone)"


def make_lama_full(with_ae=False):
    first = (
        "lazy_greedy("
            "[hff1,hlm1],"
            "preferred=[hff1,hlm1],"
            "cost_type=one,"
            "reopen_closed=false"
        ")"
    )

    phases = [first]

    for weight in [5, 3, 2, 1]:
        phases.append(
            "lazy_wastar("
                "[hff2,hlm2],"
                "preferred=[hff2,hlm2],"
                f"w={weight},"
                "cost_type=plusone"
            ")"
        )

    if with_ae:
        phases = [
            "plan_improvement("
                f"search={phase},"
                "method=action_elimination()"
            ")"
            for phase in phases
        ]

    search = (
        "iterated(["
        + ",".join(phases)
        + "],repeat_last=false)"
    )

    # Define the heuristics once and share them between phases.
    heuristic_definitions = [
        ("hlm1", (
            "eval_modify_costs("
                "landmark_sum("
                    "lm_factory=lm_reasonable_orders_hps(lm_rhw()),"
                    "pref=false"
                "),"
                "cost_type=one"
            ")"
        )),
        ("hff1", "eval_modify_costs(ff(),cost_type=one)"),
        ("hlm2", HLM_PLUSONE),
        ("hff2", HFF_PLUSONE),
    ]

    for name, expression in reversed(heuristic_definitions):
        search = f"let({name},{expression},{search})"

    return search


LAMA_FULL = make_lama_full(with_ae=False)
LAMA_FULL_AE = make_lama_full(with_ae=True)


CONFIGS = [
    (
        "1_lf",
        ["--search", LAMA_FIRST_SEARCH],
    ),
    (
        "2_lf_ae",
        ["--search", LF_AE],
    ),
    (
        "3_lama",
        ["--search", LAMA_FULL],
    ),
    (
        "4_lama_ae",
        ["--search", LAMA_FULL_AE],
    ),
]



# ---------------------------------------------------------------------------
# Build / resource limits
# ---------------------------------------------------------------------------

BUILD_OPTIONS = []


DRIVER_OPTIONS = [
    "--overall-time-limit",
    "30m",
    "--overall-memory-limit",
    "8G",
]


# ---------------------------------------------------------------------------
# Fast Downward revision
# ---------------------------------------------------------------------------
REV_NICKS = [
    ("action_elimination", ""),
]


# ---------------------------------------------------------------------------
# Report attributes
# ---------------------------------------------------------------------------

ATTRIBUTES = [
    "algorithm",
    "domain",
    "problem",
    "coverage",
    "error",
    "cost",
    "plan_length",
    "initial_plan_length",
    "final_plan_length",
    "initial_plan_cost",
    "final_plan_cost",
    "search_time",
    "plan_improvement_time",
    "total_time",
    "planner_time",
    "planner_wall_clock_time",
    "memory",
    "num_plans",
]

# ---------------------------------------------------------------------------
# Experiment
# ---------------------------------------------------------------------------

exp = project.FastDownwardExperiment(
    environment=ENV,
    revision_cache=REVISION_CACHE,
)


for config_nick, config in CONFIGS:
    for revision, revision_nick in REV_NICKS:

        if revision_nick:
            algorithm_name = f"{revision_nick}:{config_nick}"
        else:
            algorithm_name = config_nick

        exp.add_algorithm(
            algorithm_name,
            REPO,
            revision,
            config,
            build_options=BUILD_OPTIONS,
            driver_options=DRIVER_OPTIONS,
        )


exp.add_suite(
    BENCHMARKS_DIR,
    SUITE,
)


# ---------------------------------------------------------------------------
# Parsers
# ---------------------------------------------------------------------------

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.TRANSLATOR_PARSER)
exp.add_parser(exp.ANYTIME_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)

exp.add_parser(custom_parser.get_parser())

# ---------------------------------------------------------------------------
# Experiment steps
# ---------------------------------------------------------------------------

exp.add_step(
    "build",
    exp.build,
)

exp.add_step(
    "start",
    exp.start_runs,
)

exp.add_step(
    "parse",
    exp.parse,
)

exp.add_fetcher(
    name="fetch",
)


# ---------------------------------------------------------------------------
# Reports
# ---------------------------------------------------------------------------

project.add_absolute_report(
    exp,
    attributes=ATTRIBUTES,
)


# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------

exp.run_steps()