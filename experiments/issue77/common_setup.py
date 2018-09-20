# -*- coding: utf-8 -*-

import itertools
import os
import platform
import sys

from lab.environments import LocalEnvironment, MaiaEnvironment
from lab.experiment import ARGPARSER
from lab.steps import Step

from downward.experiments import DownwardExperiment, _get_rev_nick
from downward.checkouts import Translator, Preprocessor, Planner
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import CompareRevisionsReport
from downward.reports.scatter import ScatterPlotReport

from relative_scatter import RelativeScatterPlotReport


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
    return ("cluster" in node or
            node.startswith("gkigrid") or
            node in ["habakuk", "turtur"])


def is_test_run():
    return ARGS.test_run == "yes" or (ARGS.test_run == "auto" and
                                      not is_running_on_cluster())


class IssueExperiment(DownwardExperiment):
    """Wrapper for DownwardExperiment with a few convenience features."""

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

    def __init__(self, configs, suite, grid_priority=None, path=None,
                 repo=None, revisions=None, search_revisions=None,
                 test_suite=None, **kwargs):
        """Create a DownwardExperiment with some convenience features.

        *configs* must be a non-empty dict of {nick: cmdline} pairs
        that sets the planner configurations to test. ::

            IssueExperiment(configs={
                "lmcut": ["--search", "astar(lmcut())"],
                "ipdb":  ["--search", "astar(ipdb())"]})

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

        If *path* is specified, it must be the path to where the
        experiment should be built (e.g.
        /home/john/experiments/issue123/exp01/). If omitted, the
        experiment path is derived automatically from the main
        script's filename. Example::

            script = experiments/issue123/exp01.py -->
            path = experiments/issue123/data/issue123-exp01/

        If *repo* is specified, it must be the path to the root of a
        local Fast Downward repository. If omitted, the repository
        is derived automatically from the main script's path. Example::

            script = /path/to/fd-repo/experiments/issue123/exp01.py -->
            repo = /path/to/fd-repo

        If *revisions* is specified, it should be a non-empty
        list of revisions, which specify which planner versions to use
        in the experiment. The same versions are used for translator,
        preprocessor and search. ::

            IssueExperiment(revisions=["issue123", "4b3d581643"])

        If *search_revisions* is specified, it should be a non-empty
        list of revisions, which specify which search component
        versions to use in the experiment. All runs use the
        translator and preprocessor component of the first
        revision. ::

            IssueExperiment(search_revisions=["default", "issue123"])

        If you really need to specify the (translator, preprocessor,
        planner) triples manually, use the *combinations* parameter
        from the base class (might be deprecated soon). The options
        *revisions*, *search_revisions* and *combinations* can be
        freely mixed, but at least one of them must be given.

        Specify *test_suite* to set the benchmarks for experiment test
        runs. By default the first gripper task is used.

            IssueExperiment(test_suite=["depot:pfile1", "tpp:p01.pddl"])

        """

        if is_test_run():
            kwargs["environment"] = LocalEnvironment()
            suite = test_suite or self.DEFAULT_TEST_SUITE
        elif "environment" not in kwargs:
            kwargs["environment"] = MaiaEnvironment(priority=grid_priority)

        if path is None:
            path = get_data_dir()

        if repo is None:
            repo = get_repo_base()

        kwargs.setdefault("combinations", [])

        if not any([revisions, search_revisions, kwargs["combinations"]]):
            raise ValueError('At least one of "revisions", "search_revisions" '
                             'or "combinations" must be given')

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

        self._config_nicks = []
        for nick, config in configs.items():
            self.add_config(nick, config)

        self.add_suite(suite)

    @property
    def revision_nicks(self):
        # TODO: Once the add_algorithm() API is available we should get
        # rid of the call to _get_rev_nick() and avoid inspecting the
        # list of combinations by setting and saving the algorithm nicks.
        return [_get_rev_nick(*combo) for combo in self.combinations]

    def add_config(self, nick, config, timeout=None):
        DownwardExperiment.add_config(self, nick, config, timeout=timeout)
        self._config_nicks.append(nick)

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
        outfile = get_experiment_name() + "." + report.output_format
        self.add_report(report, outfile=outfile)

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
            for rev1, rev2 in itertools.combinations(self.revision_nicks, 2):
                report = CompareRevisionsReport(rev1, rev2, **kwargs)
                outfile = os.path.join(self.eval_dir,
                                       "%s-%s-%s-compare.html" %
                                       (self.name, rev1, rev2))
                report(self.eval_dir, outfile)

        self.add_step(Step("make-comparison-tables", make_comparison_tables))

    def add_scatter_plot_step(self, attributes=None, relative=False):
        """Add a step that creates scatter plots for all revision pairs.

        Create a scatter plot for each combination of attribute,
        configuration and revision pair. If *attributes* is not
        specified, a list of common scatter plot attributes is used.
        For portfolios all attributes except "cost", "coverage" and
        "plan_length" will be ignored. ::

            exp.add_scatter_plot_step(attributes=["expansions"])

        Use `relative=True` to create relative scatter plots. ::

            exp.add_scatter_plot_step(relative=True)

        """
        if attributes is None:
            attributes = self.DEFAULT_SCATTER_PLOT_ATTRIBUTES

        if relative:
            scatter_plot_class = RelativeScatterPlotReport
            scatter_dir = os.path.join(self.eval_dir, "relative-scatter")
        else:
            scatter_plot_class = ScatterPlotReport
            scatter_dir = os.path.join(self.eval_dir, "scatter")

        def is_portfolio(config_nick):
            return "fdss" in config_nick

        def make_scatter_plot(config_nick, rev1, rev2, attribute):
            name = "-".join([self.name, rev1, rev2, attribute, config_nick])
            print "Make scatter plot for", name
            algo1 = "%s-%s" % (rev1, config_nick)
            algo2 = "%s-%s" % (rev2, config_nick)
            report = scatter_plot_class(
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
