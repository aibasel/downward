# -*- coding: utf-8 -*-

import itertools
import os
import platform
import subprocess
import sys

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.experiment import ARGPARSER
from lab.steps import Step

from downward.experiments.fast_downward_experiment import FastDownwardExperiment
from downward.reports.absolute import AbsoluteReport
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
    return ("cluster" in node or
            node.startswith("gkigrid") or
            node in ["habakuk", "turtur"])


def is_test_run():
    return ARGS.test_run == "yes" or (ARGS.test_run == "auto" and
                                      not is_running_on_cluster())


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
        "plan_length",
        ]

    def __init__(self, configs, revisions, suite, build_options=None,
                 driver_options=None, grid_priority=None,
                 test_suite=None, email=None, processes=1, **kwargs):
        """Create an FastDownwardExperiment with some convenience features.
        All configs will be run on all revisions. Inherited options
        *path*, *environment* and *cache_dir* from FastDownwardExperiment
        are not supported and will be automatically set.

        *configs* must be a non-empty dict of {nick: cmdline} pairs
        that sets the planner configurations to test. nick will
        automatically get the revision prepended, e.g.
        'issue123-base-<nick>'::

            IssueExperiment(configs={
                "lmcut": ["--search", "astar(lmcut())"],
                "ipdb":  ["--search", "astar(ipdb())"]})

        *revisions* must be a non-empty list of revisions, which
        specify which planner versions to use in the experiment.
        The same versions are used for translator, preprocessor
        and search. ::

            IssueExperiment(revisions=["issue123", "4b3d581643"])

        *suite* sets the benchmarks for the experiment. It must be a
        single string or a list of strings specifying domains or
        tasks. The downward.suites module has many predefined
        suites. ::

            IssueExperiment(suite=["grid", "gripper:prob01.pddl"])

            from downward import suites
            IssueExperiment(suite=suites.suite_all())
            IssueExperiment(suite=suites.suite_satisficing_with_ipc11())
            IssueExperiment(suite=suites.suite_optimal())

        Use *grid_priority* to set the job priority for cluster
        experiments. It must be in the range [-1023, 0] where 0 is the
        highest priority. By default the priority is 0. ::

            IssueExperiment(grid_priority=-500)

        Specify *test_suite* to set the benchmarks for experiment test
        runs. By default the first gripper task is used.

            IssueExperiment(test_suite=["depot:pfile1", "tpp:p01.pddl"])

        """

        if is_test_run():
            environment = LocalEnvironment(processes=processes)
            suite = test_suite or self.DEFAULT_TEST_SUITE
        elif "environment" not in kwargs:
            environment = MaiaEnvironment(priority=grid_priority,
                                                    email=email)

        FastDownwardExperiment.__init__(self, environment=environment,
                                        **kwargs)

        # Automatically deduce the downward repository from the file
        repo = get_repo_base()
        self.algorithm_nicks = []
        self.revisions = revisions
        for nick, cmdline in configs.items():
            for rev in revisions:
                algo_nick = '%s-%s' % (rev, nick)
                self.add_algorithm(algo_nick, repo, rev, cmdline,
                                   build_options, driver_options)
                self.algorithm_nicks.append(algo_nick)

        benchmarks_dir = os.path.join(repo, 'benchmarks')
        self.add_suite(benchmarks_dir, suite)
        self.search_parsers = []

    # TODO: this method adds all search parsers. See next method.
    def _add_runs(self):
        FastDownwardExperiment._add_runs(self)
        for run in self.runs:
            for parser in self.search_parsers:
                run.add_command(parser, [parser])

    # TODO: copied adapted from downward/experiment. This method should
    # be removed when FastDownwardExperiment supports adding search parsers.
    def add_search_parser(self, path_to_parser):
        """
        Invoke script at *path_to_parser* at the end of each search run. ::

            exp.add_search_parser('path/to/parser')
        """
        if not os.path.isfile(path_to_parser):
            logging.critical('Parser %s could not be found.' % path_to_parser)
        if not os.access(path_to_parser, os.X_OK):
            logging.critical('Parser %s is not executable.' % path_to_parser)
        search_parser = 'search_parser%d' % len(self.search_parsers)
        self.add_resource(search_parser, path_to_parser)
        self.search_parsers.append(search_parser)

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
        # oufile is of the form <rev1>-<rev2>-...-<revn>.<format>
        outfile = ''
        for rev in self.revisions:
            outfile += rev
            outfile += '-'
        outfile = outfile[:len(outfile)-1]
        outfile += '.'
        outfile += report.output_format
        self.add_report(report, outfile=outfile)
        self.add_step(Step('publish-absolute-report', subprocess.call, ['publish', outfile]))

    def add_comparison_table_step(self, **kwargs):
        """Add a step that makes pairwise revision comparisons.

        Create comparative reports for all pairs of Fast Downward
        revision triples. Each report pairs up the runs of the same
        config and lists the two absolute attribute values and their
        difference for all attributes in kwargs["attributes"].

        All *kwargs* will be passed to the CompareRevisionsReport
        class. If the keyword argument *attributes* is not
        specified, a default list of attributes is used. ::

            exp.add_comparison_table_step(attributes=["coverage"])

        """
        kwargs.setdefault("attributes", self.DEFAULT_TABLE_ATTRIBUTES)

        def make_comparison_tables():
            for rev1, rev2 in itertools.combinations(self.revisions, 2):
                report = CompareRevisionsReport(rev1, rev2, **kwargs)
                outfile = os.path.join(self.eval_dir,
                                       "%s-%s-compare.html" %
                                       (rev1, rev2))
                report(self.eval_dir, outfile)

        self.add_step(Step("make-comparison-tables", make_comparison_tables))

        def publish_comparison_tables():
            for rev1, rev2 in itertools.combinations(self.revisions, 2):
                outfile = os.path.join(self.eval_dir,
                                       "%s-%s-compare.html" %
                                       (rev1, rev2))
                subprocess.call(['publish', outfile])

        self.add_step(Step('publish-comparison-reports', publish_comparison_tables))

    # TODO: test this!
    def add_scatter_plot_step(self, attributes=None):
        """Add a step that creates scatter plots for all revision pairs.

        Create a scatter plot for each combination of attribute,
        configuration and revision pair. If *attributes* is not
        specified, a list of common scatter plot attributes is used.
        For portfolios all attributes except "cost", "coverage" and
        "plan_length" will be ignored. ::

            exp.add_scatter_plot_step(attributes=["expansions"])

        """
        if attributes is None:
            attributes = self.DEFAULT_SCATTER_PLOT_ATTRIBUTES
        scatter_dir = os.path.join(self.eval_dir, "scatter")

        def is_portfolio(config_nick):
            return "fdss" in config_nick

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
            report(self.eval_dir,
                   os.path.join(scatter_dir, rev1 + "-" + rev2, name))

        def make_scatter_plots():
            for config_nick in self._config_nicks:
                if is_portfolio(config_nick):
                    valid_attributes = [
                        attr for attr in attributes
                        if attr in self.PORTFOLIO_ATTRIBUTES]
                else:
                    valid_attributes = attributes
                for rev1, rev2 in itertools.combinations(
                        self.revision_nicks, 2):
                    for attribute in valid_attributes:
                        make_scatter_plot(config_nick, rev1, rev2, attribute)

        self.add_step(Step("make-scatter-plots", make_scatter_plots))
