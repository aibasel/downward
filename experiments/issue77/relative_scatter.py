from collections import defaultdict

from downward.reports.scatter import ScatterPlotReport
from downward.reports.plot import PlotReport


EPSILON = 0.01


def get_relative_change(val1, val2):
    """
    >>> get_relative_change(10, 0)
    -999.0
    >>> get_relative_change(10, 1)
    -9.0
    >>> get_relative_change(10, 5)
    -1.0
    >>> get_relative_change(10, 10)
    0.0
    >>> get_relative_change(10, 15)
    0.5
    >>> get_relative_change(10, 20)
    1.0
    >>> get_relative_change(10, 100)
    9.0
    >>> get_relative_change(0, 10)
    999.0
    >>> get_relative_change(0, 0)
    0.0
    """
    assert val1 >= 0, val1
    assert val2 >= 0, val2
    if val1 == 0:
        val1 = EPSILON
    if val2 == 0:
        val2 = EPSILON
    if val1 > val2:
        return 1 - val1 / float(val2)
    return val2 / float(val1) - 1


class RelativeScatterPlotReport(ScatterPlotReport):
    """
    Generate a scatter plot that shows a specific attribute in two
    configurations. The attribute value in config 1 is shown on the
    x-axis and the relation to the value in config 2 on the y-axis.
    If the value for config 1 is v1 and the value for config 2 is v2,
    the plot contains the point (v1, 1 - v1/v2) if v1 > v2 and the
    point (v1, v2/v1 - 1) otherwise.
    """

    def _fill_categories(self, runs):
        # We discard the *runs* parameter.
        # Map category names to value tuples.
        categories = defaultdict(list)
        self.ylim_bottom = 0
        self.ylim_top = 0
        for (domain, problem), runs in self.problem_runs.items():
            if len(runs) != 2:
                continue
            run1, run2 = runs
            assert (run1['config'] == self.configs[0] and
                    run2['config'] == self.configs[1])
            val1 = run1.get(self.attribute)
            val2 = run2.get(self.attribute)
            if val1 is None or val2 is None:
                continue
            category = self.get_category(run1, run2)
            assert val1 >= 0, (domain, problem, self.configs[0], val1)
            assert val2 >= 0, (domain, problem, self.configs[1], val2)
            x = val1
            y = get_relative_change(val1, val2)
            categories[category].append((x, y))
            self.ylim_bottom = min(self.ylim_bottom, y)
            self.ylim_top = max(self.ylim_top, y)
        self.ylim_bottom *= 1.1
        self.ylim_top *= 1.1
        return categories

    def _set_scales(self, xscale, yscale):
        # ScatterPlots use log-scaling on the x-axis by default.
        default_xscale = 'log'
        if self.attribute and self.attribute in self.LINEAR:
            default_xscale = 'linear'
        PlotReport._set_scales(self, xscale or default_xscale, 'linear')
