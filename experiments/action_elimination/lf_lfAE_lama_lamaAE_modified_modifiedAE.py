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
        time_limit_per_task="00:31:00",
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


CONFIGS = {
    "0_lama_first": [
        "--search",
        "let(hlm, eval_modify_costs("
        "landmark_sum(lm_factory=lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=one),"
        "let(hff, eval_modify_costs(ff(),cost_type=one),"
        "lazy_greedy([hff,hlm],preferred=[hff,hlm],"
        "cost_type=one,reopen_closed=false)))",
    ],

    "1_lama_first_ae": [
        "--search",
        "let(hlm, eval_modify_costs("
        "landmark_sum(lm_factory=lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=one),"
        "let(hff, eval_modify_costs(ff(),cost_type=one),"
        "plan_improvement("
        "search=lazy_greedy([hff,hlm],preferred=[hff,hlm],"
        "cost_type=one,reopen_closed=false),"
        "improvement_method=action_elimination())))",
    ],

    "2_iterated_lama": [
        "--search",
        "--if-unit-cost",
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "let(hff, ff(),"
        "iterated(["
        "lazy_greedy([hff,hlm],preferred=[hff,hlm]),"
        "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),"
        "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),"
        "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),"
        "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)"
        "],repeat_last=true)))",
        "--if-non-unit-cost",
        "let(hlm1, eval_modify_costs("
        "landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=one),"
        "let(hff1, eval_modify_costs(ff(),cost_type=one),"
        "let(hlm2, eval_modify_costs("
        "landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=plusone),"
        "let(hff2, eval_modify_costs(ff(),cost_type=plusone),"
        "iterated(["
        "lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],"
        "cost_type=one,reopen_closed=false),"
        "lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],"
        "reopen_closed=false),"
        "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),"
        "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),"
        "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),"
        "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)"
        "],repeat_last=true)))))",
        "--always",
    ],

    "3_iterated_lama_ae": [
        "--search",
        "--if-unit-cost",
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "let(hff, ff(),"
        "iterated(["
        "plan_improvement("
        "search=lazy_greedy([hff,hlm],preferred=[hff,hlm]),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff,hlm],preferred=[hff,hlm],w=5),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1),"
        "improvement_method=action_elimination())"
        "],repeat_last=true)))",
        "--if-non-unit-cost",
        "let(hlm1, eval_modify_costs("
        "landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=one),"
        "let(hff1, eval_modify_costs(ff(),cost_type=one),"
        "let(hlm2, eval_modify_costs("
        "landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=plusone),"
        "let(hff2, eval_modify_costs(ff(),cost_type=plusone),"
        "iterated(["
        "plan_improvement("
        "search=lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],"
        "cost_type=one,reopen_closed=false),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],"
        "reopen_closed=false),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=5),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1),"
        "improvement_method=action_elimination())"
        "],repeat_last=true)))))",
        "--always",
    ],
    "4_lama_no_w5": [
        "--search",
        "--if-unit-cost",
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "let(hff, ff(),"
        "iterated(["
        "lazy_greedy([hff,hlm],preferred=[hff,hlm]),"
        "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),"
        "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),"
        "lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1)"
        "],repeat_last=true)))",
        "--if-non-unit-cost",
        "let(hlm1, eval_modify_costs("
        "landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=one),"
        "let(hff1, eval_modify_costs(ff(),cost_type=one),"
        "let(hlm2, eval_modify_costs("
        "landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=plusone),"
        "let(hff2, eval_modify_costs(ff(),cost_type=plusone),"
        "iterated(["
        "lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],"
        "cost_type=one,reopen_closed=false),"
        "lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],"
        "reopen_closed=false),"
        "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),"
        "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),"
        "lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1)"
        "],repeat_last=true)))))",
        "--always",
    ],
    "5_lama_no_w5_ae": [
        "--search",
        "--if-unit-cost",
        "let(hlm, landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "let(hff, ff(),"
        "iterated(["
        "plan_improvement("
        "search=lazy_greedy([hff,hlm],preferred=[hff,hlm]),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff,hlm],preferred=[hff,hlm],w=3),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff,hlm],preferred=[hff,hlm],w=2),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff,hlm],preferred=[hff,hlm],w=1),"
        "improvement_method=action_elimination())"
        "],repeat_last=true)))",
        "--if-non-unit-cost",
        "let(hlm1, eval_modify_costs("
        "landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=one),"
        "let(hff1, eval_modify_costs(ff(),cost_type=one),"
        "let(hlm2, eval_modify_costs("
        "landmark_sum(lm_reasonable_orders_hps(lm_rhw()),pref=false),"
        "cost_type=plusone),"
        "let(hff2, eval_modify_costs(ff(),cost_type=plusone),"
        "iterated(["
        "plan_improvement("
        "search=lazy_greedy([hff1,hlm1],preferred=[hff1,hlm1],"
        "cost_type=one,reopen_closed=false),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_greedy([hff2,hlm2],preferred=[hff2,hlm2],"
        "reopen_closed=false),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=3),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=2),"
        "improvement_method=action_elimination()),"
        "plan_improvement("
        "search=lazy_wastar([hff2,hlm2],preferred=[hff2,hlm2],w=1),"
        "improvement_method=action_elimination())"
        "],repeat_last=true)))))",
        "--always",
    ],
}



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


for config_nick, config in CONFIGS.items():
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