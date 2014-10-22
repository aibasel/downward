# -*- coding: utf-8 -*-

import os.path
import platform

from lab.environments import MaiaEnvironment
from lab.steps import Step

from downward.checkouts import Translator, Preprocessor, Planner
from downward.experiments import DownwardExperiment
from downward.reports.compare import CompareRevisionsReport
from downward.reports.scatter import ScatterPlotReport


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
    while True:
        if os.path.exists(os.path.join(path, ".hg")):
            return path
        path = os.path.dirname(path)


def build_combos_with_names(repo, combinations, revisions, search_revisions):
    """Build (combos, combo_names) lists for the given planner revisions.

    combos and combo_names are parallel lists, where combos contains
    (Translator, Preprocessor, Search) triples and combo_names are the names
    for the respective combinations that lab uses internally.

    See MyExperiment.__init__ for documentation of the parameters
    combinations, revisions and search_revisions."""
    combos = []
    names = []
    def build(*rev_triple):
        combo, name = build_combo_with_name(repo, *rev_triple)
        combos.append(combo)
        names.append(name)

    for triple in combinations or []:
        build(triple)
    for rev in revisions or []:
        build(rev, rev, rev)
    for rev in search_revisions or []:
        build(search_revisions[0], search_revisions[0], rev)

    return combos, names


def build_combo_with_name(repo, trans_rev, preprocess_rev, search_rev):
    """Generate a tuple (combination, name) for the given revisions.

    combination is a (Translator, Preprocessor, Search) tuple
    and name is the name that lab uses to refer to it."""
    # TODO: In the future, it would be nice if we didn't need the name
    # information any more, as it is somewhat of an implementation
    # detail.
    combo = (Translator(repo, trans_rev),
             Preprocessor(repo, preprocess_rev),
             Planner(repo, search_rev))
    if trans_rev == preprocess_rev == search_rev:
        name = str(search_rev)
    else:
        name = "%s-%s-%s" % (trans_rev, preprocess_rev, search_rev)
    return combo, name


def is_on_grid():
    """Returns True if the current machine is on the maia grid.

    Implemented by checking if host name ends with ".cluster".
    """
    return platform.node().endswith(".cluster")


class MyExperiment(DownwardExperiment):
    DEFAULT_TEST_SUITE = [
        "zenotravel:pfile1",
        "zenotravel:pfile2",
        ]

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
                 combinations=None, suite=None, do_test_run="auto",
                 test_suite=DEFAULT_TEST_SUITE, **kwargs):
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

        If "combinations" is specified, it should be a non-empty list
        of revision triples of the form (translator_rev,
        preprocessor_rev, search_rev).

        If "revisions" is specified, it should be a non-empty
        list of revisions, which specify which planner versions to use
        in the experiment. The same versions are used for translator,
        preprocessor and search.

        If "search_revisions" is specified, it should be a non-empty
        list of revisions, which specify which search component
        versions to use in the experiment. All experiments use the
        translator and preprocessor component of the first
        revision.

        It is possible to specify a mixture of"combinations",
        "revisions" and "search_revisions".

        If "suite" is specified, it should specify a problem suite.

        If "do_test_run" is true, the "grid_priority" and
        "environment" (from the base class) arguments are ignored and
        a local experiment with default arguments is run instead. In
        this case, the "suite" argument is replaced by the "test_suite"
        argument.

        If "do_test_run" is the string "auto" (the default), then
        do_test_run is set to False when run on a grid machine and
        to True otherwise. A grid machine is identified as one whose
        node name ends with ".cluster".
        """

        if do_test_run == "auto":
            do_test_run = not is_on_grid()

        if do_test_run:
            # In a test run, overwrite certain arguments.
            grid_priority = None
            kwargs.pop("environment", None)
            suite = test_suite

        if grid_priority is not None and "environment" not in kwargs:
            kwargs["environment"] = MaiaEnvironment(priority=grid_priority)

        if path is None:
            path = get_data_dir()

        if repo is None:
            repo = get_repo_base()

        combinations, self._combination_names = build_combos_with_names(
            repo=repo,
            combinations=combinations,
            revisions=revisions,
            search_revisions=search_revisions)
        kwargs["combinations"] = combinations

        DownwardExperiment.__init__(self, path=path, repo=repo, **kwargs)

        if configs is not None:
            for nick, config in configs.items():
                self.add_config(nick, config)

        if suite is not None:
            self.add_suite(suite)

        self._report_prefix = get_experiment_name()

    def add_comparison_table_step(self, attributes=None):
        revisions = self._combination_names
        if len(revisions) != 2:
            # TODO: Should generalize this by offering a general
            # grouping function and then comparing any pair of
            # settings in the same group.
            raise NotImplementedError("need two revisions")
        if attributes is None:
            attributes = self.DEFAULT_TABLE_ATTRIBUTES
        report = CompareRevisionsReport(*revisions, attributes=attributes)
        self.add_report(report, outfile="%s-compare.html" % self._report_prefix)

    def add_scatter_plot_step(self, attributes=None):
        if attributes is None:
            attributes = self.DEFAULT_SCATTER_PLOT_ATTRIBUTES
        revisions = self._combination_names
        if len(revisions) != 2:
            # TODO: Should generalize this by offering a general
            # grouping function and then comparing any pair of
            # settings in the same group.
            raise NotImplementedError("need two revisions")
        scatter_dir = os.path.join(self.eval_dir, "scatter")
        def make_scatter_plots():
            configs = [conf[0] for conf in self.configs]
            for nick in configs:
                config_before = "%s-%s" % (revisions[0], nick)
                config_after = "%s-%s" % (revisions[1], nick)
                for attribute in attributes:
                    name = "%s-%s-%s" % (self._report_prefix, attribute, nick)
                    report = ScatterPlotReport(
                        filter_config=[config_before, config_after],
                        attributes=[attribute],
                        get_category=lambda run1, run2: run1["domain"],
                        legend_location=(1.3, 0.5))
                    report(self.eval_dir, os.path.join(scatter_dir, name))

        self.add_step(Step("make-scatter-plots", make_scatter_plots))
