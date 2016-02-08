# -*- coding: utf-8 -*-

import itertools
import os
import platform
import subprocess
import sys

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.experiment import ARGPARSER
from lab.steps import Step
from lab import tools

from downward.experiment import FastDownwardExperiment
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import CompareConfigsReport
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
    return tools.get_script_path()


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
    return (
        "cluster" in node or
        node.startswith("gkigrid") or
        node in ["habakuk", "turtur"])


def is_test_run():
    return ARGS.test_run == "yes" or (
        ARGS.test_run == "auto" and not is_running_on_cluster())


def get_algo_nick(revision, config_nick):
    return "{revision}-{config_nick}".format(**locals())


class IssueConfig(object):
    """Hold information about a planner configuration.

    See FastDownwardExperiment.add_algorithm() for documentation of the
    constructor's options.

    """
    def __init__(self, nick, component_options,
                 build_options=None, driver_options=None):
        self.nick = nick
        self.component_options = component_options
        self.build_options = build_options
        self.driver_options = driver_options


class IssueExperiment(FastDownwardExperiment):
    """Wrapper for FastDownwardExperiment with a few convenience features."""

    DEFAULT_TEST_SUITE = "gripper:prob01.pddl"

    DEFAULT_TABLE_ATTRIBUTES = [
        "cost",
        "coverage",
        "error",
        "evaluations",
        "expansions",
        "expansions_until_last_jump",
        "generated",
        "memory",
        "quality",
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
        "error",
        "plan_length",
        "run_dir",
        ]

    def __init__(self, suite, revisions=[], configs={}, grid_priority=None,
                 path=None, test_suite=None, email=None, processes=None,
                 **kwargs):
        """Create a DownwardExperiment with some convenience features.

        If *revisions* is specified, it should be a non-empty
        list of revisions, which specify which planner versions to use
        in the experiment. The same versions are used for translator,
        preprocessor and search. ::

            IssueExperiment(revisions=["issue123", "4b3d581643"], ...)

        *configs* must be a non-empty list of IssueConfig objects. ::

            IssueExperiment(..., configs=[
                IssueConfig("ff", ["--search", "eager_greedy(ff())"]),
                IssueConfig(
                    "lama", [],
                    driver_options=["--alias", "seq-sat-lama-2011"]),
            ])

        *suite* sets the benchmarks for the experiment. It must be a
        single string or a list of strings specifying domains or
        tasks. The downward.suites module has many predefined
        suites. ::

            IssueExperiment(..., suite=["grid", "gripper:prob01.pddl"])

            from downward import suites
            IssueExperiment(..., suite=suites.suite_all())
            IssueExperiment(..., suite=suites.suite_satisficing_with_ipc11())
            IssueExperiment(..., suite=suites.suite_optimal())

        Use *grid_priority* to set the job priority for cluster
        experiments. It must be in the range [-1023, 0] where 0 is the
        highest priority. By default the priority is 0. ::

            IssueExperiment(..., grid_priority=-500)

        If *path* is specified, it must be the path to where the
        experiment should be built (e.g.
        /home/john/experiments/issue123/exp01/). If omitted, the
        experiment path is derived automatically from the main
        script's filename. Example::

            script = experiments/issue123/exp01.py -->
            path = experiments/issue123/data/issue123-exp01/

        Specify *test_suite* to set the benchmarks for experiment test
        runs. By default the first gripper task is used.

            IssueExperiment(..., test_suite=["depot:pfile1", "tpp:p01.pddl"])

        If *email* is specified, it should be an email address. This
        email address will be notified upon completion of the experiments
        if it is run on the cluster.
        """

        if is_test_run():
            kwargs["environment"] = LocalEnvironment(processes=processes)
            suite = test_suite or self.DEFAULT_TEST_SUITE
        elif "environment" not in kwargs:
            kwargs["environment"] = MaiaEnvironment(
                priority=grid_priority, email=email)

        path = path or get_data_dir()

        FastDownwardExperiment.__init__(self, path=path, **kwargs)

        repo = get_repo_base()
        for rev in revisions:
            for config in configs:
                self.add_algorithm(
                    get_algo_nick(rev, config.nick),
                    repo,
                    rev,
                    config.component_options,
                    build_options=config.build_options,
                    driver_options=config.driver_options)

        self.add_suite(os.path.join(repo, "benchmarks"), suite)

        self._revisions = revisions
        self._configs = configs

    @classmethod
    def _is_portfolio(cls, config_nick):
        return "fdss" in config_nick

    @classmethod
    def get_supported_attributes(cls, config_nick, attributes):
        if cls._is_portfolio(config_nick):
            return [attr for attr in attributes
                    if attr in cls.PORTFOLIO_ATTRIBUTES]
        return attributes

    def add_absolute_report_step(self, **kwargs):
        """Add step that makes an absolute report.

        Absolute reports are useful for experiments that don't
        compare revisions.

        The report is written to the experiment evaluation directory.

        All *kwargs* will be passed to the AbsoluteReport class. If
        the keyword argument *attributes* is not specified, a
        default list of attributes is used. ::

            exp.add_absolute_report_step(attributes=["coverage"])

        """
        kwargs.setdefault("attributes", self.DEFAULT_TABLE_ATTRIBUTES)
        report = AbsoluteReport(**kwargs)
        outfile = os.path.join(self.eval_dir,
                               get_experiment_name() + "." +
                               report.output_format)
        self.add_report(report, outfile=outfile)
        self.add_step(Step('publish-absolute-report',
                           subprocess.call,
                           ['publish', outfile]))

    def add_comparison_table_step(self, **kwargs):
        """Add a step that makes pairwise revision comparisons.

        Create comparative reports for all pairs of Fast Downward
        revisions. Each report pairs up the runs of the same config and
        lists the two absolute attribute values and their difference
        for all attributes in kwargs["attributes"].

        All *kwargs* will be passed to the CompareConfigsReport class.
        If the keyword argument *attributes* is not specified, a
        default list of attributes is used. ::

            exp.add_comparison_table_step(attributes=["coverage"])

        """
        kwargs.setdefault("attributes", self.DEFAULT_TABLE_ATTRIBUTES)

        def make_comparison_tables():
            for rev1, rev2 in itertools.combinations(self._revisions, 2):
                compared_configs = []
                for config in self._configs:
                    config_nick = config.nick
                    compared_configs.append(
                        ("%s-%s" % (rev1, config_nick),
                         "%s-%s" % (rev2, config_nick),
                         "Diff (%s)" % config_nick))
                report = CompareConfigsReport(compared_configs, **kwargs)
                outfile = os.path.join(
                    self.eval_dir,
                    "%s-%s-%s-compare" % (self.name, rev1, rev2)
                    + "." + report.output_format)
                report(self.eval_dir, outfile)

        def publish_comparison_tables():
            for rev1, rev2 in itertools.combinations(self._revisions, 2):
                outfile = os.path.join(
                    self.eval_dir,
                    "%s-%s-%s-compare" % (self.name, rev1, rev2)
                    + ".html")
                subprocess.call(['publish', outfile])

        self.add_step(Step("make-comparison-tables", make_comparison_tables))
        self.add_step(Step("publish-comparison-tables", publish_comparison_tables))

    def add_scatter_plot_step(self, attributes=None):
        """Add a step that creates scatter plots for all revision pairs.

        Create a scatter plot for each combination of attribute,
        configuration and revisions pair. If *attributes* is not
        specified, a list of common scatter plot attributes is used.
        For portfolios all attributes except "cost", "coverage" and
        "plan_length" will be ignored. ::

            exp.add_scatter_plot_step(attributes=["expansions"])

        """
        if attributes is None:
            attributes = self.DEFAULT_SCATTER_PLOT_ATTRIBUTES
        scatter_dir = os.path.join(self.eval_dir, "scatter")

        def make_scatter_plot(config_nick, rev1, rev2, attribute):
            name = "-".join([self.name, rev1, rev2, attribute, config_nick])
            print "Make scatter plot for", name
            algo1 = "%s-%s" % (rev1, config_nick)
            algo2 = "%s-%s" % (rev2, config_nick)
            report = ScatterPlotReport(
                filter_config=[algo1, algo2],
                attributes=[attribute],
                get_category=lambda run1, run2: run1["domain"],
                legend_location=(1.3, 0.5))
            report(
                self.eval_dir,
                os.path.join(scatter_dir, rev1 + "-" + rev2, name))

        def make_scatter_plots():
            for config in self._configs:
                for rev1, rev2 in itertools.combinations(self._revisions, 2):
                    for attribute in self.get_supported_attributes(
                            config.nick, attributes):
                        make_scatter_plot(config.nick, rev1, rev2, attribute)

        self.add_step(Step("make-scatter-plots", make_scatter_plots))
