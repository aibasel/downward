#! /usr/bin/python3
import os
import sys

sys.argv.append("bdist_wheel")

home = os.getcwd()

# go to translator root folder
os.chdir("../../translate")

# setup for bdist_wheel
from setuptools import setup, find_packages
setup(
    name = 'translator',
    version='1.0',
    # Use one of the below approach to define package and/or module names:

    #if there are only handful of modules placed in root directory, and no packages/directories exist then can use below syntax
#     packages=[''], #have to import modules directly in code after installing this wheel, like import mod2 (respective file name in this case is mod2.py) - no direct use of distribution name while importing

    #can list down each package names - no need to keep __init__.py under packages / directories
    packages=['translator', 'translator/pddl', 'translator/pddl_parser'], #importing is like: from package1 import mod2, or import package1.mod2 as m2

    # this approach automatically finds out all directories (packages) - those must contain a file named __init__.py (can be empty)
    # packages=find_packages(), #include/exclude arguments take * as wildcard, . for any sub-package names
)

# move files to home directory and remove unnecessary files
import shutil
shutil.rmtree('build/')
shutil.rmtree('translator.egg-info/')
for file in os.listdir('dist'):
    shutil.move('dist/' + file, os.path.join(home, file))
shutil.rmtree('dist/')
