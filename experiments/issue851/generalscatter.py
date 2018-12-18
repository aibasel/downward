#! /usr/bin/env python
# -*- coding: utf-8 -*-

from collections import defaultdict
import logging
import math
import os

from lab import tools

from downward.reports.plot import MatplotlibPlot, Matplotlib, PgfPlots, \
    PlotReport, MIN_AXIS


class ScatterMatplotlib(Matplotlib):
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
            # TODO: assert that both are linear or log
            plot_size = max(report.x_missing_val * 1.01, report.y_missing_val * 1.01)
        else:
            plot_size = max(report.x_missing_val * 1.5, report.y_missing_val * 1.5)

        # Plot a diagonal black line. Starting at (0,0) often raises errors.
        axes.plot([0.001, plot_size], [0.001, plot_size], 'k')

        axes.set_xlim(report.xlim_left or -1, report.xlim_right or plot_size)
        axes.set_ylim(report.ylim_bottom or -1, report.ylim_top or plot_size)
        # axes.set_xlim(report.xlim_left, report.xlim_right)
        # axes.set_ylim(report.ylim_bottom, report.ylim_top)

        for axis in [axes.xaxis, axes.yaxis]:
            # MatplotlibPlot.change_axis_formatter(
                # axis, report.missing_val if report.show_missing else None)
            MatplotlibPlot.change_axis_formatter(axes.xaxis,
                report.x_missing_val if report.show_missing else None)
            MatplotlibPlot.change_axis_formatter(axes.yaxis,
                report.y_missing_val if report.show_missing else None)
        return has_points


class ScatterPgfPlots(PgfPlots):
    @classmethod
    def _format_coord(cls, coord):
        def format_value(v):
            return str(v) if isinstance(v, int) else '%f' % v
        return '(%s, %s)' % (format_value(coord[0]), format_value(coord[1]))

    @classmethod
    def _get_plot(cls, report):
        lines = []
        options = cls._get_axis_options(report)
        lines.append('\\begin{axis}[%s]' % cls._format_options(options))
        for category, coords in sorted(report.categories.items()):
            plot = {'only marks': True}
            lines.append(
                '\\addplot+[%s] coordinates {\n%s\n};' % (
                    cls._format_options(plot),
                    ' '.join(cls._format_coord(c) for c in coords)))
            if category:
                lines.append('\\addlegendentry{%s}' % category)
            elif report.has_multiple_categories:
                # None is treated as the default category if using multiple
                # categories. Add a corresponding entry to the legend.
                lines.append('\\addlegendentry{default}')
        # Add black line.
        start = min(report.min_x, report.min_y)
        if report.xlim_left is not None:
            start = min(start, report.xlim_left)
        if report.ylim_bottom is not None:
            start = min(start, report.ylim_bottom)
        end = max(report.max_x, report.max_y)
        if report.xlim_right:
            end = max(end, report.xlim_right)
        if report.ylim_top:
            end = max(end, report.ylim_top)
        if report.show_missing:
            end = max(end, report.missing_val)
        lines.append(
            '\\addplot[color=black] coordinates {(%f, %f) (%d, %d)};' %
            (start, start, end, end))
        lines.append('\\end{axis}')
        return lines

    @classmethod
    def _get_axis_options(cls, report):
        opts = PgfPlots._get_axis_options(report)
        # Add line for missing values.
        for axis in ['x', 'y']:
            opts['extra %s ticks' % axis] = report.missing_val
            opts['extra %s tick style' % axis] = 'grid=major'
        return opts

class GeneralScatterPlotReport(PlotReport):
    """
    Generate a scatter plot for a specific attribute.
    """
    def __init__(self, x_algo, y_algo, x_attribute, y_attribute, show_missing=True, get_category=None, **kwargs):
        """
        See :class:`.PlotReport` for inherited arguments.

        The keyword argument *attributes* must contain exactly one
        attribute.

        Use the *filter_algorithm* keyword argument to select exactly
        two algorithms.

        If only one of the two algorithms has a value for a run, only
        add a coordinate if *show_missing* is True.

        *get_category* can be a function that takes **two** runs
        (dictionaries of properties) and returns a category name. This
        name is used to group the points in the plot. If there is more
        than one group, a legend is automatically added. Runs for which
        this function returns None are shown in a default category and
        are not contained in the legend. For example, to group by
        domain:

        >>> def domain_as_category(run1, run2):
        ...     # run2['domain'] has the same value, because we always
        ...     # compare two runs of the same problem.
        ...     return run1['domain']

        Example grouping by difficulty:

        >>> def improvement(run1, run2):
        ...     time1 = run1.get('search_time', 1800)
        ...     time2 = run2.get('search_time', 1800)
        ...     if time1 > time2:
        ...         return 'better'
        ...     if time1 == time2:
        ...         return 'equal'
        ...     return 'worse'

        >>> from downward.experiment import FastDownwardExperiment
        >>> exp = FastDownwardExperiment()
        >>> exp.add_report(ScatterPlotReport(
        ...     attributes=['search_time'],
        ...     get_category=improvement))

        Example comparing the number of expanded states for two
        algorithms:

        >>> exp.add_report(ScatterPlotReport(
        ...         attributes=["expansions_until_last_jump"],
        ...         filter_algorithm=["algorithm-1", "algorithm-2"],
        ...         get_category=domain_as_category,
        ...         format="png",  # Use "tex" for pgfplots output.
        ...         ),
        ...     name="scatterplot-expansions")

        """
        # If the size has not been set explicitly, make it a square.
        matplotlib_options = kwargs.get('matplotlib_options', {})
        matplotlib_options.setdefault('figure.figsize', [8, 8])
        kwargs['matplotlib_options'] = matplotlib_options
        PlotReport.__init__(self, **kwargs)
        if not self.attribute:
            logging.critical('ScatterPlotReport needs exactly one attribute')
        # By default all values are in the same category.
        self.get_category = get_category or (lambda run1, run2: None)
        self.show_missing = show_missing
        self.xlim_left = self.xlim_left or MIN_AXIS
        self.ylim_bottom = self.ylim_bottom or MIN_AXIS
        if self.output_format == 'tex':
            self.writer = ScatterPgfPlots
        else:
            self.writer = ScatterMatplotlib
        self.x_algo = x_algo
        self.y_algo = y_algo
        self.x_attribute = x_attribute
        self.y_attribute = y_attribute

    def _set_scales(self, xscale, yscale):
        PlotReport._set_scales(self, xscale or self.attribute.scale or 'log', yscale)
        if self.xscale != self.yscale:
            logging.critical('Scatterplots must use the same scale on both axes.')

    def _get_missing_val(self, max_value, scale):
        """
        Separate the missing values by plotting them at (max_value * 10)
        rounded to the next power of 10.
        """
        assert max_value is not None
        # HACK!
        max_value = 1800
        if scale == 'linear':
            return max_value * 1.1
        return int(10 ** math.ceil(math.log10(max_value)))

    def _handle_none_values(self, X, Y, replacement_x, replacement_y):
        assert len(X) == len(Y), (X, Y)
        if self.show_missing:
            return ([x if x is not None else replacement_x for x in X],
                    [y if y is not None else replacement_y for y in Y])
        return zip(*[(x, y) for x, y in zip(X, Y) if x is not None and y is not None])

    def _fill_categories(self, runs):
        # We discard the *runs* parameter.
        # Map category names to value tuples
        categories = defaultdict(list)
        x_count = 0
        y_count = 0
        x_none_count = 0
        y_none_count = 0
        for (domain, problem), runs in self.problem_runs.items():
            run1 = next((run for run in runs if run['algorithm'] == self.x_algo), None)
            run2 = next((run for run in runs if run['algorithm'] == self.y_algo), None)
            if run1 is None or run2 is None:
                continue
            assert (run1['algorithm'] == self.x_algo and
                    run2['algorithm'] == self.y_algo)
            val1 = run1.get(self.x_attribute)
            val2 = run2.get(self.y_attribute)
            x_count += 1
            y_count += 1
            if val1 is None:
                x_none_count += 1
            if val2 is None:
                y_none_count += 1
            # print val1, val2
            if val1 is None and val2 is None:
                continue
            category = self.get_category(run1, run2)
            categories[category].append((val1, val2))
        # print x_count, y_count
        # print x_none_count, y_none_count
        # print len(categories[None])
        # print categories[None]
        return categories

    def _get_limit(self, varlist, limit_type):
        assert limit_type == 'max' or limit_type == 'min'
        varlist = [x for x in varlist if x is not None]
        if(limit_type == 'max'):
            return max(varlist)
        else:
            return min(varlist)

    def _get_plot_size(self, missing_val, scale):
        if scale == 'linear':
            return missing_val * 1.01
        else:
            return missing_val * 1.25

    def _prepare_categories(self, categories):
        categories = PlotReport._prepare_categories(self, categories)

        # Find max-value to fit plot and to draw missing values.
        # self.missing_val = self._get_missing_val(max(self.max_x, self.max_y))
        self.x_missing_val = self._get_missing_val(self.max_x, self.xscale)
        self.y_missing_val = self._get_missing_val(self.max_y, self.yscale)
        # print self.x_missing_val, self.y_missing_val

        # set minima
        self.xlim_left = self._get_limit([self.xlim_left, self.min_x],'min')
        self.ylim_bottom = self._get_limit([self.ylim_bottom, self.min_y],'min')

        # set maxima
        x_plot_size = y_plot_size = None
        if self.show_missing:
            x_plot_size = self._get_plot_size(self.x_missing_val, self.xscale)
            y_plot_size = self._get_plot_size(self.y_missing_val, self.yscale)
        self.xlim_right = self._get_limit([self.xlim_right, self.max_x, x_plot_size], 'max')
        self.ylim_top = self._get_limit([self.ylim_top, self.max_y, y_plot_size], 'max')

        # self.diagonal_start = self.diagonal_end = None
        # if self.show_diagonal:
            # self.diagonal_start = max(self.xlim_left, self.ylim_bottom)
            # self.diagonal_end = min(self.xlim_right, self.ylim_top)

        new_categories = {}
        for category, coords in categories.items():
            X, Y = zip(*coords)
            # X, Y = self._handle_none_values(X, Y, self.missing_val)
            X, Y = self._handle_none_values(X, Y, self.x_missing_val, self.y_missing_val)
            coords = zip(X, Y)
            new_categories[category] = coords
        # print len(new_categories[None])
        # print new_categories[None]
        return new_categories

    def write(self):
        if not (len(self.algorithms) == 1 and self.x_algo == self.algorithms[0] and self.y_algo == self.algorithms[0]):
            logging.critical(
                'Scatter plots need exactly 1 algorithm that must match x_algo and y_algo: %s, %s, %s' % (self.algorithms, self.x_algo, self.y_algo))
        self.xlabel = self.xlabel or self.x_algo + ": " + self.x_attribute
        self.ylabel = self.ylabel or self.y_algo + ": " + self.y_attribute

        suffix = '.' + self.output_format
        if not self.outfile.endswith(suffix):
            self.outfile += suffix
        tools.makedirs(os.path.dirname(self.outfile))
        self._write_plot(self.runs.values(), self.outfile)
