import os
import platform
import shutil
from subprocess import call

from downward.experiments import DownwardExperiment, Translator, Preprocessor, Planner
from downward.reports.absolute import AbsoluteReport
from downward.reports.compare import CompareRevisionsReport
from downward.reports.compare import CompareConfigsReport
from downward.reports.scatter import ScatterPlotReport
from lab.steps import Step
from lab.environments import MaiaEnvironment
from lab.environments import LocalEnvironment

# Environment settings
NODE = platform.node()
REMOTE = 'cluster' in NODE
SCP_LOGIN = 'maia'

# Remote and local path settings
# Note: do not use ~ here because we need both home-paths for scp-steps
REMOTE_EXP_PATH = '/infai/sieverss/experiments/executed'
LOCAL_EXP_PATH = '/home/silvan/experiments/executed'

REMOTE_PYTHON = '/infai/sieverss/bin/python2.7'
LOCAL_PYTHON = 'python2.7'

CACHE_DIR = os.path.expanduser('~/experiments/cache_dir')

# Default settings
DEFAULT_ATTRIBUTES = [
    'cost',
    'coverage',
    'expansions_until_last_jump',
    'initial_h_value',
    'memory',
    'run_dir',
    #'search_returncode',
    'search_time',
    'total_time',
]
EVAL_EXP_GEN = [
    'evaluations',
    'expansions',
    'generated',
]
SCORE_ATTRIBUTES = [
    'score_evaluations',
    'score_expansions',
    'score_memory',
    'score_search_time',
    'score_total_time',
]

class StandardDownwardExperiment(DownwardExperiment):
    def __init__(self, name, repo, revisions, limits,
                 compilation_option, priority, queue):
        self.attributes = DEFAULT_ATTRIBUTES
        remote_exp_path = os.path.join(REMOTE_EXP_PATH, name)
        local_exp_path = os.path.join(LOCAL_EXP_PATH, name)

        if REMOTE:
            path = remote_exp_path
            environment = MaiaEnvironment(priority=priority,
                                          queue=queue,
                                          email='silvan.sievers@unibas.ch')
            python = REMOTE_PYTHON
        else:
            path = local_exp_path
            environment = LocalEnvironment(processes=4)
            python = LOCAL_PYTHON
            if limits is None:
                limits = { 'search_time': 30 }

        if revisions:
            combinations = []
            for revision in revisions:
                if isinstance(revision, str):
                    combinations.append((Translator(repo, rev=revision),
                                         Preprocessor(repo, rev=revision),
                                         Planner(repo, rev=revision)))
                else:
                    assert isinstance(revision, tuple)
                    assert len(revision) == 3
                    combinations.append((Translator(repo, rev=revision[0]),
                                         Preprocessor(repo, rev=revision[1]),
                                         Planner(repo, rev=revision[2])))
        else:
            combinations = None

        DownwardExperiment.__init__(self, path, repo, environment=environment,
                                    combinations=combinations, limits=limits,
                                    cache_dir=CACHE_DIR)
        self.set_path_to_python(python)
        if compilation_option:
            self.compilation_options.append(compilation_option)

        # From here, add various default steps
        if REMOTE:
            # Compress the experiment directory
            self.add_step(Step.zip_exp_dir(self))

        if not REMOTE:
            # Copy the zipped experiment directory to local directory
            self.add_step(Step('scp-zipped-exp-dir',
                               call,
                               ['scp', '-r',
                               '%s:%s.tar.bz2' % (SCP_LOGIN, remote_exp_path),
                               '%s.tar.bz2' % local_exp_path]))

            # Copy the properties file from the evaluation directory to local directory
            self.add_step(Step('scp-eval-dir',
                               call,
                               ['mkdir -p %s-eval && scp -r %s:%s-eval/properties %s-eval/properties'
                               % (local_exp_path, SCP_LOGIN, remote_exp_path, local_exp_path)], shell=True))

            # Unzip the experiment directory
            self.add_step(Step.unzip_exp_dir(self))

        # Remove the experiment (and evaluation) directory
        self.add_step(Step('remove-exp-dir', shutil.rmtree,
                           self.path, ignore_errors=True))
        self.add_step(Step('remove-eval-dir', shutil.rmtree,
                           self.eval_dir, ignore_errors=True))

    def add_score_attributes(self):
        self.attributes.extend(SCORE_ATTRIBUTES)

    def add_eval_exp_gen(self):
        self.attributes.extend(EVAL_EXP_GEN)

    def add_extra_attributes(self, attributes):
        self.attributes.extend(attributes)

    def add_absolute_report(self, name='', attributes=None, **filters):
        if name is not '':
            name = name + '-'
        name = name + 'abs'
        if attributes is None:
            attributes = self.attributes
        abs_file = os.path.join(self.eval_dir, '%s-%s.html' % (self.name, name))
        self.add_report(AbsoluteReport(attributes=attributes,
                                       **filters),
                        name='report-%s' % name, outfile=abs_file)
        self.add_step(Step('publish-%s' % name, call, ['publish', abs_file]))

    def add_configs_report(self, compared_configs, name='', attributes=None, **filters):
        if name is not '':
            name = name + '-'
        name = name + 'comp'
        if attributes is None:
            attributes = self.attributes
        conf_file = os.path.join(self.eval_dir, '%s-%s.html' % (self.name, name))
        self.add_report(CompareConfigsReport(attributes=attributes,
                                             compared_configs=compared_configs,
                                             **filters),
                        name='report-%s' % name, outfile=conf_file)
        self.add_step(Step('publish-%s' % name, call, ['publish', conf_file]))

    def add_revisions_report(self, rev1, rev2, name='', attributes=None, **filters):
        if name is not '':
            name = name + '-'
        name = name + 'rev'
        if attributes is None:
            attributes = self.attributes
        rev_file = os.path.join(self.eval_dir, '%s-%s.html' % (self.name, name))
        self.add_report(CompareRevisionsReport(rev1,
                                               rev2,
                                               attributes=attributes,
                                               **filters),
                        name='report-%s' % name, outfile=rev_file)
        self.add_step(Step('publish-%s' % name, call, ['publish', rev_file]))

    def add_latex_report(self, name='', attributes=None, **filters):
        if attributes is None:
            attributes = self.attributes
        tex_file = os.path.join(self.eval_dir, '%s-%s.tex' % (self.name, name))
        self.add_report(AbsoluteReport(attributes=attributes,
                                       format='tex',
                                       **filters),
                        name='report-%s' % name, outfile=tex_file)

    def add_scatter_report(self, name='', attributes=None, **filters):
        if name is not '':
            name = name + '-'
        name = name + 'scatter'
        if attributes is None:
            attributes = self.attributes
        scatter_file = os.path.join(self.eval_dir, '%s-%s' % (self.name, name))
        self.add_report(ScatterPlotReport(attributes=attributes,
                                          **filters),
                        name='report-%s' % name, outfile=scatter_file)


def get_exp(script_path, repo, suite, configs, revisions=None,
            limits=None, compilation_option=None, priority=None,
            queue=None, benchmarks_dir=None):
    exp_name = os.path.splitext(os.path.basename(script_path))[0]
    project_name = os.path.basename(os.path.dirname(script_path))
    name=os.path.join(project_name, exp_name)

    exp = StandardDownwardExperiment(name=name, repo=repo,
                                     revisions=revisions,
                                     limits=limits,
                                     compilation_option=compilation_option,
                                     priority=priority, queue=queue)

    exp.add_suite(suite, benchmarks_dir)
    for nick, config in configs.iteritems():
        exp.add_config(nick, config)
    return exp
