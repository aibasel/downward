import os
from string import Template

current_dir = os.path.dirname(os.path.realpath(__file__))


def load_template(name):
    with open(os.path.join(current_dir, '..', 'templates', "{}.tpl".format(name)), "r") as f:
        return Template(f.read())


def fill_template(template_name, target, parameters):
    tpl = load_template(template_name)
    string = tpl.substitute(parameters)
    with open(target, "w") as f:
        f.write(string)
    print("Written file {}".format(target))
