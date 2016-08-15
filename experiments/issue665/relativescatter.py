# -*- coding: utf-8 -*-
#
# downward uses the lab package to conduct experiments with the
# Fast Downward planning system.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from collections import defaultdict
import os

from lab import tools

from matplotlib import ticker

from downward.reports.scatter import ScatterPlotReport
from downward.reports.plot import PlotReport, Matplotlib, MatplotlibPlot


# TODO: handle outliers

# TODO: this is mostly copied from ScatterMatplotlib (scatter.py)
class RelativeScatterMatplotlib(Matplotlib):
    @classmethod
    def _plot(cls, report, axes, categories, styles):
        # Display grid
        axes.grid(b=True, linestyle='-', color='0.75')

        has_points = False
        # Generate the scatter plots
        for category, coords in sorted(categories.items()):
            X, Y = zip(*coords)
            axes.scatter(X, Y, s=42, label=category, **styles[category])
            if X and Y:
                has_points = True

        if report.xscale == 'linear' or report.yscale == 'linear':
            plot_size = report.missing_val * 1.01
        else:
            plot_size = report.missing_val * 1.25

        # make 5 ticks above and below 1
        yticks = []
        tick_step = report.ylim_top**(1/5.0)
        for i in xrange(-5, 6):
            yticks.append(tick_step**i)
        axes.set_yticks(yticks)
        axes.get_yaxis().set_major_formatter(ticker.ScalarFormatter())

        axes.set_xlim(report.xlim_left or -1, report.xlim_right or plot_size)
        axes.set_ylim(report.ylim_bottom or -1, report.ylim_top or plot_size)

        for axis in [axes.xaxis, axes.yaxis]:
            MatplotlibPlot.change_axis_formatter(axis,
                                report.missing_val if report.show_missing else None)
        return has_points



class RelativeScatterPlotReport(ScatterPlotReport):
    """
    Generate a scatter plot that shows how a specific attribute in two
    configurations. The attribute value in config 1 is shown on the
    x-axis and the relation to the value in config 2 on the y-axis.
    """

    def __init__(self, show_missing=True, get_category=None, **kwargs):
        ScatterPlotReport.__init__(self, show_missing, get_category, **kwargs)
        if self.output_format == 'tex':
            raise "not supported"
        else:
            self.writer = RelativeScatterMatplotlib


    def _fill_categories(self, runs):
        # We discard the *runs* parameter.
        # Map category names to value tuples
        categories = defaultdict(list)
        self.ylim_bottom = 2
        self.ylim_top = 0.5
        self.xlim_left = float("inf")
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
            assert val1 > 0, (domain, problem, self.configs[0], val1)
            assert val2 > 0, (domain, problem, self.configs[1], val2)
            x = val1
            y = val2 / float(val1)

            categories[category].append((x, y))

            self.ylim_top = max(self.ylim_top, y)
            self.ylim_bottom = min(self.ylim_bottom, y)
            self.xlim_left = min(self.xlim_left, x)

        # center around 1
        if self.ylim_bottom < 1:
            self.ylim_top = max(self.ylim_top, 1 / float(self.ylim_bottom))
        if self.ylim_top > 1:
            self.ylim_bottom = min(self.ylim_bottom, 1 / float(self.ylim_top))
        return categories

    def _set_scales(self, xscale, yscale):
        # ScatterPlots use log-scaling on the x-axis by default.
        default_xscale = 'log'
        if self.attribute and self.attribute in self.LINEAR:
            default_xscale = 'linear'
        PlotReport._set_scales(self, xscale or default_xscale, 'log')
