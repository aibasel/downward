# -*- coding: utf-8 -*-

import itertools
import os
import platform
import sys

from lab.environments import MaiaEnvironment
from lab.experiment import ARGPARSER
from lab.steps import Step

from downward.experiments import DownwardExperiment, _get_rev_nick
from downward.checkouts import Translator, Preprocessor, Planner
from downward.reports.compare import CompareRevisionsReport
from downward.reports.scatter import ScatterPlotReport


def parse_args():
    ARGPARSER.add_argument(
        "--test",
        choices=["yes", "no", "auto"],
        default="auto",
        dest="test_run",
        help="test experiment locally on a small suite if --test=yes or "
             "--test=auto and we are not on a cluster")
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

    Derived from the absolute filename of the main script, e.g.
    "/ham/spam/eggs.py" => "spam-eggs"."""
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

    Search upwards in the directory tree from the main script until a
    directory with a subdirectory named ".hg" is found.

    Abort if the repo base cannot be found."""
    path = os.path.abspath(get_script_dir())
    while os.path.dirname(path) != path:
        if os.path.exists(os.path.join(path, ".hg")):
            return path
        path = os.path.dirname(path)
    sys.exit("repo base could not be found")


def is_running_on_cluster():
    node = platform.node()
    return (node.endswith("cluster") or
            node.startswith("gkigrid") or
            node in ["habakuk", "turtur"])


def is_test_run():
    return ARGS.test_run == "yes" or (ARGS.test_run == "auto" and
                                      not is_running_on_cluster())


class IssueExperiment(DownwardExperiment):
    """Wrapper for DownwardExperiment with a few convenience features."""

    # TODO: Once we have reference results, we should add "quality" here.
    # TODO: Add something about errors/exit codes.
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
        "evaluations",
        "expansions",
        "expansions_until_last_jump",
        "initial_h_value",
        "memory",
        "search_time",
        "total_time",
        ]

    PORTFOLIO_ATTRIBUTES = [
        "cost",
        "coverage",
        "plan_length",
        ]

    def __init__(self, configs=None, grid_priority=None, path=None,
                 repo=None, revisions=None, search_revisions=None,
                 suite=None, **kwargs):
        """Create a DownwardExperiment with some convenience features.

        If "configs" is specified, it should be a dict of {nick:
        cmdline} pairs that sets the planner configurations to test. ::

            IssueExperiment(configs={
                "lmcut": ["--search", "astar(lmcut())"],
                "ipdb":  ["--search", "astar(ipdb())"]})

        Use "grid_priority" to set the job priority for cluster
        experiments. It must be in the range [-1023, 0] where 0 is the
        highest priority. By default the priority is 0. ::

            IssueExperiment(grid_priority=-500)

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
        "search_revisions" can be freely mixed."""

        if is_test_run():
            # Use LocalEnvironment.
            kwargs["environment"] = None
            suite = "gripper:prob01.pddl"
        elif "environment" not in kwargs:
            kwargs["environment"] = MaiaEnvironment(priority=grid_priority)

        if path is None:
            path = get_data_dir()

        if repo is None:
            repo = get_repo_base()

        kwargs.setdefault("combinations", [])

        if revisions:
            kwargs["combinations"].extend([
                (Translator(repo, rev),
                 Preprocessor(repo, rev),
                 Planner(repo, rev))
                for rev in revisions])

        if search_revisions:
            base_rev = search_revisions[0]
            # Use the same nick for all parts to get short revision nick.
            kwargs["combinations"].extend([
                (Translator(repo, base_rev, nick=rev),
                 Preprocessor(repo, base_rev, nick=rev),
                 Planner(repo, rev, nick=rev))
                for rev in search_revisions])

        DownwardExperiment.__init__(self, path=path, repo=repo, **kwargs)

        configs = configs or {}
        for nick, config in configs.items():
            self.add_config(nick, config)
        self._config_nicks = configs.keys()

        if suite is not None:
            self.add_suite(suite)

    @property
    def revision_nicks(self):
        # TODO: Use add_algorithm() API once it's available.
        return [_get_rev_nick(*combo) for combo in self.combinations]

    def add_comparison_table_step(self, attributes=None):
        """
        Add a step that creates comparison tables for all pairs of revisions.
        """
        if attributes is None:
            attributes = self.DEFAULT_TABLE_ATTRIBUTES

        def make_comparison_tables():
            for rev1, rev2 in itertools.combinations(self.revision_nicks, 2):
                report = CompareRevisionsReport(rev1, rev2, attributes=attributes)
                outfile = os.path.join(self.eval_dir,
                                       "%s-%s-compare.html" % (rev1, rev2))
                report(self.eval_dir, outfile)

        self.add_step(Step("make-comparison-tables", make_comparison_tables))

    def add_scatter_plot_step(self, attributes=None):
        """
        Add a step that creates scatter plots for all pairs of revisions.
        """
        if attributes is None:
            attributes = self.DEFAULT_SCATTER_PLOT_ATTRIBUTES
        scatter_dir = os.path.join(self.eval_dir, "scatter")

        def is_portfolio(config_nick):
            return "fdss" in config_nick

        def make_scatter_plots():
            for config_nick in self._config_nicks:
                for rev1, rev2 in itertools.combinations(self.revision_nicks, 2):
                    algo1 = "%s-%s" % (rev1, config_nick)
                    algo2 = "%s-%s" % (rev2, config_nick)
                    if is_portfolio(config_nick):
                        valid_attributes = [
                            attr for attr in attributes
                            if attr in self.PORTFOLIO_ATTRIBUTES]
                    else:
                        valid_attributes = attributes
                    for attribute in valid_attributes:
                        name = "-".join([rev1, rev2, attribute, config_nick])
                        print "Make scatter plot for", name
                        report = ScatterPlotReport(
                            filter_config=[algo1, algo2],
                            attributes=[attribute],
                            get_category=lambda run1, run2: run1["domain"],
                            legend_location=(1.3, 0.5))
                        report(self.eval_dir, os.path.join(scatter_dir, name))

        self.add_step(Step("make-scatter-plots", make_scatter_plots))
