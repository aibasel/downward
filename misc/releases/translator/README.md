# Fast Downward translator

This package contains the translator of the [Fast Downward planning
system](https://www.fast-downward.org). It parses planning tasks specified in
the Planning Domain Definition Language PDDL, performs several transformations
and generates the
[`output.sas` format](https://www.fast-downward.org/latest/documentation/translator-output-format/)
that serves as the input for the search component of the planning system.

At the moment, you can call it with `python3 -m downward.translate`.

