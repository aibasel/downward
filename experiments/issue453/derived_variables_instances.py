
from downward.reports import PlanningReport

DERIVED_VARIABLES_SUITE=['airport-adl',
                         'assembly',
                         'miconic-fulladl',
                         'openstacks',
                         'openstacks-opt08-adl',
                         'openstacks-sat08-adl',
                         'optical-telegraphs',
                         'philosophers',
                         'psr-large',
                         'psr-middle',
                         'trucks']

class DerivedVariableInstances(PlanningReport):
    def get_text(self):
        selected_runs = []
        for (dom, prob), runs in self.problem_runs.items():
            for run in runs:
                if run.get("translator_derived_variables") > 0:
                    selected_runs.append((dom, prob))

        return "\n".join(["{}:{},".format(*item) for item in selected_runs])
