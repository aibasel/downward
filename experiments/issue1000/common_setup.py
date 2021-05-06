# -*- coding: utf-8 -*-

import itertools
import os
import platform
import subprocess
import sys

from lab.experiment import ARGPARSER
from lab import tools

from downward.experiment import FastDownwardExperiment
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import ComparativeReport
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


DEFAULT_OPTIMAL_SUITE = [
    'agricola-opt18-strips', 'airport', 'barman-opt11-strips',
    'barman-opt14-strips', 'blocks', 'childsnack-opt14-strips',
    'data-network-opt18-strips', 'depot', 'driverlog',
    'elevators-opt08-strips', 'elevators-opt11-strips',
    'floortile-opt11-strips', 'floortile-opt14-strips', 'freecell',
    'ged-opt14-strips', 'grid', 'gripper', 'hiking-opt14-strips',
    'logistics00', 'logistics98', 'miconic', 'movie', 'mprime',
    'mystery', 'nomystery-opt11-strips', 'openstacks-opt08-strips',
    'openstacks-opt11-strips', 'openstacks-opt14-strips',
    'openstacks-strips', 'organic-synthesis-opt18-strips',
    'organic-synthesis-split-opt18-strips', 'parcprinter-08-strips',
    'parcprinter-opt11-strips', 'parking-opt11-strips',
    'parking-opt14-strips', 'pathways-noneg', 'pegsol-08-strips',
    'pegsol-opt11-strips', 'petri-net-alignment-opt18-strips',
    'pipesworld-notankage', 'pipesworld-tankage', 'psr-small', 'rovers',
    'satellite', 'scanalyzer-08-strips', 'scanalyzer-opt11-strips',
    'snake-opt18-strips', 'sokoban-opt08-strips',
    'sokoban-opt11-strips', 'spider-opt18-strips', 'storage',
    'termes-opt18-strips', 'tetris-opt14-strips',
    'tidybot-opt11-strips', 'tidybot-opt14-strips', 'tpp',
    'transport-opt08-strips', 'transport-opt11-strips',
    'transport-opt14-strips', 'trucks-strips', 'visitall-opt11-strips',
    'visitall-opt14-strips', 'woodworking-opt08-strips',
    'woodworking-opt11-strips', 'zenotravel']

DEFAULT_SATISFICING_SUITE = [
    'agricola-sat18-strips', 'airport', 'assembly',
    'barman-sat11-strips', 'barman-sat14-strips', 'blocks',
    'caldera-sat18-adl', 'caldera-split-sat18-adl', 'cavediving-14-adl',
    'childsnack-sat14-strips', 'citycar-sat14-adl',
    'data-network-sat18-strips', 'depot', 'driverlog',
    'elevators-sat08-strips', 'elevators-sat11-strips',
    'flashfill-sat18-adl', 'floortile-sat11-strips',
    'floortile-sat14-strips', 'freecell', 'ged-sat14-strips', 'grid',
    'gripper', 'hiking-sat14-strips', 'logistics00', 'logistics98',
    'maintenance-sat14-adl', 'miconic', 'miconic-fulladl',
    'miconic-simpleadl', 'movie', 'mprime', 'mystery',
    'nomystery-sat11-strips', 'nurikabe-sat18-adl', 'openstacks',
    'openstacks-sat08-adl', 'openstacks-sat08-strips',
    'openstacks-sat11-strips', 'openstacks-sat14-strips',
    'openstacks-strips', 'optical-telegraphs',
    'organic-synthesis-sat18-strips',
    'organic-synthesis-split-sat18-strips', 'parcprinter-08-strips',
    'parcprinter-sat11-strips', 'parking-sat11-strips',
    'parking-sat14-strips', 'pathways', 'pathways-noneg',
    'pegsol-08-strips', 'pegsol-sat11-strips', 'philosophers',
    'pipesworld-notankage', 'pipesworld-tankage', 'psr-large',
    'psr-middle', 'psr-small', 'rovers', 'satellite',
    'scanalyzer-08-strips', 'scanalyzer-sat11-strips', 'schedule',
    'settlers-sat18-adl', 'snake-sat18-strips', 'sokoban-sat08-strips',
    'sokoban-sat11-strips', 'spider-sat18-strips', 'storage',
    'termes-sat18-strips', 'tetris-sat14-strips',
    'thoughtful-sat14-strips', 'tidybot-sat11-strips', 'tpp',
    'transport-sat08-strips', 'transport-sat11-strips',
    'transport-sat14-strips', 'trucks', 'trucks-strips',
    'visitall-sat11-strips', 'visitall-sat14-strips',
    'woodworking-sat08-strips', 'woodworking-sat11-strips',
    'zenotravel']


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
    directory with a subdirectory named ".git" is found.

    Abort if the repo base cannot be found."""
    path = os.path.abspath(get_script_dir())
    while os.path.dirname(path) != path:
        if os.path.exists(os.path.join(path, ".git")):
            return path
        path = os.path.dirname(path)
    sys.exit("repo base could not be found")


def is_running_on_cluster():
    node = platform.node()
    return node.endswith(".scicore.unibas.ch") or node.endswith(".cluster.bc2.ch")


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
    """Subclass of FastDownwardExperiment with some convenience features."""

    DEFAULT_TEST_SUITE = ["depot:p01.pddl", "gripper:prob01.pddl"]

    DEFAULT_TABLE_ATTRIBUTES = [
        "cost",
        "coverage",
        "error",
        "evaluations",
        "expansions",
        "expansions_until_last_jump",
        "initial_h_value",
        "generated",
        "memory",
        "planner_memory",
        "planner_time",
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

    def __init__(self, revisions=None, configs=None, path=None, **kwargs):
        """

        You can either specify both *revisions* and *configs* or none
        of them. If they are omitted, you will need to call
        exp.add_algorithm() manually.

        If *revisions* is given, it must be a non-empty list of
        revision identifiers, which specify which planner versions to
        use in the experiment. The same versions are used for
        translator, preprocessor and search. ::

            IssueExperiment(revisions=["issue123", "4b3d581643"], ...)

        If *configs* is given, it must be a non-empty list of
        IssueConfig objects. ::

            IssueExperiment(..., configs=[
                IssueConfig("ff", ["--search", "eager_greedy(ff())"]),
                IssueConfig(
                    "lama", [],
                    driver_options=["--alias", "seq-sat-lama-2011"]),
            ])

        If *path* is specified, it must be the path to where the
        experiment should be built (e.g.
        /home/john/experiments/issue123/exp01/). If omitted, the
        experiment path is derived automatically from the main
        script's filename. Example::

            script = experiments/issue123/exp01.py -->
            path = experiments/issue123/data/issue123-exp01/

        """

        path = path or get_data_dir()

        FastDownwardExperiment.__init__(self, path=path, **kwargs)

        if (revisions and not configs) or (not revisions and configs):
            raise ValueError(
                "please provide either both or none of revisions and configs")

        for rev in revisions:
            for config in configs:
                self.add_algorithm(
                    get_algo_nick(rev, config.nick),
                    get_repo_base(),
                    rev,
                    config.component_options,
                    build_options=config.build_options,
                    driver_options=config.driver_options)

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

        Absolute reports are useful for experiments that don't compare
        revisions.

        The report is written to the experiment evaluation directory.

        All *kwargs* will be passed to the AbsoluteReport class. If the
        keyword argument *attributes* is not specified, a default list
        of attributes is used. ::

            exp.add_absolute_report_step(attributes=["coverage"])

        """
        kwargs.setdefault("attributes", self.DEFAULT_TABLE_ATTRIBUTES)
        report = AbsoluteReport(**kwargs)
        outfile = os.path.join(
            self.eval_dir,
            get_experiment_name() + "." + report.output_format)
        self.add_report(report, outfile=outfile)
        self.add_step(
            'publish-absolute-report', subprocess.call, ['publish', outfile])

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
                report = ComparativeReport(compared_configs, **kwargs)
                outfile = os.path.join(
                    self.eval_dir,
                    "%s-%s-%s-compare.%s" % (
                        self.name, rev1, rev2, report.output_format))
                report(self.eval_dir, outfile)

        def publish_comparison_tables():
            for rev1, rev2 in itertools.combinations(self._revisions, 2):
                outfile = os.path.join(
                    self.eval_dir,
                    "%s-%s-%s-compare.html" % (self.name, rev1, rev2))
                subprocess.call(["publish", outfile])

        self.add_step("make-comparison-tables", make_comparison_tables)
        self.add_step(
            "publish-comparison-tables", publish_comparison_tables)

    def add_scatter_plot_step(self, relative=False, attributes=None, additional=[]):
        """Add step creating (relative) scatter plots for all revision pairs.

        Create a scatter plot for each combination of attribute,
        configuration and revisions pair. If *attributes* is not
        specified, a list of common scatter plot attributes is used.
        For portfolios all attributes except "cost", "coverage" and
        "plan_length" will be ignored. ::

            exp.add_scatter_plot_step(attributes=["expansions"])

        """
        if relative:
            scatter_dir = os.path.join(self.eval_dir, "scatter-relative")
            step_name = "make-relative-scatter-plots"
        else:
            scatter_dir = os.path.join(self.eval_dir, "scatter-absolute")
            step_name = "make-absolute-scatter-plots"
        if attributes is None:
            attributes = self.DEFAULT_SCATTER_PLOT_ATTRIBUTES

        def make_scatter_plot(config_nick, rev1, rev2, attribute, config_nick2=None):
            name = "-".join([self.name, rev1, rev2, attribute, config_nick])
            if config_nick2 is not None:
                name += "-" + config_nick2
            print("Make scatter plot for", name)
            algo1 = get_algo_nick(rev1, config_nick)
            algo2 = get_algo_nick(rev2, config_nick if config_nick2 is None else config_nick2)
            report = ScatterPlotReport(
                filter_algorithm=[algo1, algo2],
                attributes=[attribute],
                relative=relative,
                get_category=lambda run1, run2: run1["domain"])
            report(
                self.eval_dir,
                os.path.join(scatter_dir, rev1 + "-" + rev2, name))

        def make_scatter_plots():
            for config in self._configs:
                print(config)
                for rev1, rev2 in itertools.combinations(self._revisions, 2):
                    print(rev1, rev2)
                    for attribute in self.get_supported_attributes(
                            config.nick, attributes):
                        print(attribute)
                        make_scatter_plot(config.nick, rev1, rev2, attribute)
            for nick1, nick2, rev1, rev2, attribute in additional:
                make_scatter_plot(nick1, rev1, rev2, attribute, config_nick2=nick2)

        self.add_step(step_name, make_scatter_plots)
