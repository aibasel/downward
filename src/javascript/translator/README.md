# Translator in the browser 
We offer to build the translator as python wheel.
This is done by executing the `setup.py` script.
**Pyodide** can then be used to load the python wheel in the browser.

## How to execute the translator using pyodide
The following steps are exemplarily presented in `usage_example.html` and `usage_example.js`
- you need to serve the JS file through a server; python can be used for that: `python -m http.server`
- in your javascript file, load the current pyodide version
- load pyodide's *micropip* package
- load the *.pddl* files into two javascript strings
- use micropip to install the translator-wheel created by `setup.py` into the python environment
- store the pddl strings to the browser's virtual filesystem using python
- import and execute the translator in python:
	```Python
from translator.translate import run
run(["domain.pddl", "problem.pddl", "--sas-file", "output.sas"])
	```
- load the result from the python environment to the javascript environment for further use