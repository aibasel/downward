# -*- coding: utf-8 -*-
#
# Downward Lab uses the Lab package to conduct experiments with the
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

from operator import itemgetter

import logging

from lab.reports import Table, DynamicDataModule

from downward.reports import PlanningReport



class SortedReport(PlanningReport):
    def __init__(self, sort_spec, **kwargs):
        PlanningReport.__init__(self, **kwargs)
        self._sort_spec = sort_spec

    def get_markup(self):
        """
        Return `txt2tags <http://txt2tags.org/>`_ markup for the report.

        """
        table = Table()
        row_sort_module = RowSortModule(self._sort_spec)
        table.dynamic_data_modules.append(row_sort_module)
        for run_id, run in self.props.items():
            row = {}
            for key, value in run.items():
                if key not in self.attributes:
                    continue
                if isinstance(value, (list, tuple)):
                    key = '-'.join([str(item) for item in value])
                row[key] = value
            table.add_row(run_id, row)
        return str(table)

class RowSortModule(DynamicDataModule):
    def __init__(self, sort_spec):
        self._sort_spec = sort_spec

    def modify_printable_row_order(self, table, row_order):
        col_names = [None] + table.col_names

        entries = []
        for row_name in row_order:
            if row_name == 'column names (never printed)':
                continue
            entry = [row_name] + table.get_row(row_name)
            entries.append(tuple(entry))

        for attribute, desc in reversed(self._sort_spec):
            index = col_names.index(attribute)
            reverse = desc == 'desc'

            entries.sort(key=itemgetter(index), reverse=reverse)

        new_row_order = ['column names (never printed)'] + [i[0] for i in entries]

        return new_row_order
