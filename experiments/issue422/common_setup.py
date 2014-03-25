# -*- coding: utf-8 -*-

import itertools
import os
import platform
import sys

from lab.environments import MaiaEnvironment
from lab.experiment import ARGPARSER
from lab.steps import Step

from downward.experiments import DownwardExperiment
from downward.reports.compare import CompareRevisionsReport
from downward.reports.scatter import ScatterPlotReport


def parse_args():
    ARGPARSER.add_argument(
        "--test",
        choices=["yes", "no", "auto"],
        default="auto",
        dest="test_run",
        help="test setup on small suite locally")
    return ARGPARSER.parse_args()

ARGS = parse_args()


def get_script():
    """Get file name of main script."""
    import __main__
    return __main__.__file__


def get_script_dir():
    """Get directory of main script.

    Usually a relative directory (depends on how it was called by the user.)"""
    return os.path.dirname(get_script())


def get_experiment_name():
    """Get name for experiment.

    Derived from the filename of the main script, e.g.
    "/ham/spam/eggs.py" => "eggs"."""
    script = os.path.abspath(get_script())
    script_dir = os.path.basename(os.path.dirname(script))
    script_base = os.path.splitext(os.path.basename(script))[0]
    return "%s-%s" % (script_dir, script_base)


def get_data_dir():
    """Get data dir for the experiment.

    This is the subdirectory "data" of the directory containing
    the main script."""
    return os.path.join(get_script_dir(), "data", get_experiment_name())


def get_repo_base():
    """Get base directory of the repository, as an absolute path.

    Found by searching upwards in the directory tree from the main
    script until a directory with a subdirectory named ".hg" is found."""
    path = os.path.abspath(get_script_dir())
    while path != '/':
        if os.path.exists(os.path.join(path, ".hg")):
            return path
        path = os.path.dirname(path)


def is_running_on_cluster():
    node = platform.node()
    return (node.endswith("cluster") or
            node.startswith("gkigrid") or
            node in ["habakuk", "turtur"])


def is_test_run():
    return ARGS.test_run == "yes" or (ARGS.test_run == "auto" and
                                      not is_running_on_cluster())


class MyExperiment(DownwardExperiment):
    DEFAULT_TABLE_ATTRIBUTES = [
        "cost",
        "coverage",
        "evaluations",
        "expansions",
        "expansions_until_last_jump",
        "generated",
        "memory",
        "run_dir",
        "score_evaluations",
        "score_expansions",
        "score_generated",
        "score_memory",
        "score_search_time",
        "score_total_time",
        "search_time",
        "total_time",
        ]

    DEFAULT_SCATTER_PLOT_ATTRIBUTES = [
        "total_time",
        "search_time",
        "memory",
        "expansions_until_last_jump",
        ]

    """Wrapper for DownwardExperiment with a few convenience features."""

    def __init__(self, configs=None, grid_priority=None, path=None,
                 repo=None, revisions=None, search_revisions=None,
                 suite=None, **kwargs):
        """Create a DownwardExperiment with some convenience features.

        If "configs" is specified, it should be a dict of {nick:
        cmdline} pairs that sets the planner configurations to test.

        If "grid_priority" is specified and no environment is
        specifically requested in **kwargs, use the maia environment
        with the specified priority.

        If "path" is not specified, the experiment data path is
        derived automatically from the main script's filename.

        If "repo" is not specified, the repository base is derived
        automatically from the main script's path.

        If "revisions" is specified, it should be a non-empty
        list of revisions, which specify which planner versions to use
        in the experiment. The same versions are used for translator,
        preprocessor and search.

        If "search_revisions" is specified, it should be a non-empty
        list of revisions, which specify which search component
        versions to use in the experiment. All experiments use the
        translator and preprocessor component of the first
        revision.

        If "suite" is specified, it should specify a problem suite.

        Options "combinations" (from the base class), "revisions" and
        "search_revisions" can be freely mixed. However, only
        "revisions" and "search_revisions" will be included in the
        comparison table and the scatter plots."""

        configs = configs or {}

        if grid_priority is not None and "environment" not in kwargs:
            kwargs["environment"] = MaiaEnvironment(priority=grid_priority)

        if path is None:
            path = get_data_dir()

        if repo is None:
            repo = get_repo_base()
            if repo is None:
                # If the script is called by another program (e.g. by a
                # profiler), we cannot find the repo base.
                sys.exit("repo base could not be found")

        DownwardExperiment.__init__(self, path=path, repo=repo, **kwargs)

        combinations = kwargs.get("combinations", [])
        revisions = revisions or []
        search_revisions = search_revisions or []

        if combinations or (not revisions and not search_revisions):
            for config_nick, config in configs.items():
                self.add_config(config_nick, config)

        for rev in revisions:
            for config_nick, config in configs.items():
                algo_nick = rev + "-" + config_nick
                self.add_config(algo_nick, config,
                                preprocess_rev=rev, search_rev=rev)

        if search_revisions:
            base_rev = search_revisions[0]
            for search_rev in search_revisions:
                for config_nick, config in configs.items():
                    algo_nick = search_rev + "-" + config_nick
                    self.add_config(algo_nick, config,
                                    preprocess_rev=base_rev,
                                    search_rev=search_rev)

        if suite is not None:
            self.add_suite(suite)

        self._compared_revs = revisions + search_revisions
        self._config_nicks = configs.keys()

    def add_comparison_table_step(self, attributes=None):
        if attributes is None:
            attributes = self.DEFAULT_TABLE_ATTRIBUTES

        def make_comparison_tables():
            for rev1, rev2 in itertools.combinations(self._compared_revs, 2):
                report = CompareRevisionsReport(rev1, rev2, attributes=attributes)
                outfile = os.path.join(self.eval_dir,
                                       "%s-%s-compare.html" % (rev1, rev2))
                report(self.eval_dir, outfile)

        self.add_step(Step("make-comparison-tables", make_comparison_tables))

    def add_scatter_plot_step(self, attributes=None):
        if attributes is None:
            attributes = self.DEFAULT_SCATTER_PLOT_ATTRIBUTES
        scatter_dir = os.path.join(self.eval_dir, "scatter")

        def make_scatter_plots():
            for config_nick in self._config_nicks:
                for rev1, rev2 in itertools.combinations(self._compared_revs, 2):
                    algo1 = "%s-%s" % (rev1, config_nick)
                    algo2 = "%s-%s" % (rev2, config_nick)
                    for attribute in attributes:
                        name = "-".join([rev1, rev2, attribute, config_nick])
                        report = ScatterPlotReport(
                            filter_config=[algo1, algo2],
                            attributes=[attribute],
                            get_category=lambda run1, run2: run1["domain"],
                            legend_location=(1.3, 0.5))
                        report(self.eval_dir, os.path.join(scatter_dir, name))

        self.add_step(Step("make-scatter-plots", make_scatter_plots))
