from downward.reports import PlanningReport

class CSVReport(PlanningReport):
    def get_text(self):
        sep = " "
        lines = [sep.join(self.attributes)]
        for runs in self.problem_runs.values():
            for run in runs:
                lines.append(sep.join([str(run.get(attribute, "nan"))
                                       for attribute in self.attributes]))
        return "\n".join(lines)
