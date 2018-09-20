# -*- coding: utf-8 -*-

import os.path

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
        "search_revisions" are mutually exclusive."""

        if grid_priority is not None and "environment" not in kwargs:
            kwargs["environment"] = MaiaEnvironment(priority=grid_priority)

        if path is None:
            path = get_data_dir()

        if repo is None:
            repo = get_repo_base()

        num_rev_opts_specified = (
            int(revisions is not None) +
            int(search_revisions is not None) +
            int(kwargs.get("combinations") is not None))

        if num_rev_opts_specified > 1:
            raise ValueError('must specify exactly one of "revisions", '
                             '"search_revisions" or "combinations"')

        # See add_comparison_table_step for more on this variable.
        self._HACK_revisions = revisions

        if revisions is not None:
            if not revisions:
                raise ValueError("revisions cannot be empty")
            combinations = [(Translator(repo, rev),
                             Preprocessor(repo, rev),
                             Planner(repo, rev))
                            for rev in revisions]
            kwargs["combinations"] = combinations

        if search_revisions is not None:
            if not search_revisions:
                raise ValueError("search_revisions cannot be empty")
            base_rev = search_revisions[0]
            translator = Translator(repo, base_rev)
            preprocessor = Preprocessor(repo, base_rev)
            combinations = [(translator, preprocessor, Planner(repo, rev))
                            for rev in search_revisions]
            kwargs["combinations"] = combinations

        DownwardExperiment.__init__(self, path=path, repo=repo, **kwargs)

        if configs is not None:
            for nick, config in configs.items():
                self.add_config(nick, config)

        if suite is not None:
            self.add_suite(suite)

        self._report_prefix = get_experiment_name()

    def add_comparison_table_step(self, attributes=None):
        revisions = self._HACK_revisions
        if revisions is None:
            # TODO: It's not clear to me what a "revision" in the
            # overall context of the code really is, e.g. when keeping
            # the translator and preprocessor method fixed and only
            # changing the search component. It's also not really
            # clear to me how the interface of the Compare... reports
            # works and how to use it more generally. Hence the
            # present hack.

            # Ideally, this method should look at the table columns we
            # have (defined by planners and planner configurations),
            # pair them up in a suitable way, either controlled by a
            # convenience parameter or a more general grouping method,
            # and then use this to define which pairs go together.
            raise NotImplementedError(
                "only supported when specifying revisions in __init__")

        if attributes is None:
            attributes = self.DEFAULT_TABLE_ATTRIBUTES
        report = CompareRevisionsReport(*revisions, attributes=attributes)
        self.add_report(report, outfile="%s-compare.html" % self._report_prefix)

    def add_scatter_plot_step(self, attributes=None):
        if attributes is None:
            attributes = self.DEFAULT_SCATTER_PLOT_ATTRIBUTES
        revisions = self._HACK_revisions
        if revisions is None:
            # TODO: See add_comparison_table_step.
            raise NotImplementedError(
                "only supported when specifying revisions in __init__")
        if len(revisions) != 2:
            # TODO: Should generalize this, too, by offering a general
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
