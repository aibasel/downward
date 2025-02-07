<img src="misc/images/fast-downward.svg" width="800" alt="Fast Downward">

Fast Downward is a domain-independent classical planning system.

Copyright 2003-2025 Fast Downward contributors (see below).

For further information:
- Fast Downward website: <https://www.fast-downward.org>
- Report a bug or file an issue: <https://issues.fast-downward.org>
- Fast Downward mailing list: <https://groups.google.com/forum/#!forum/fast-downward>
- Fast Downward main repository: <https://github.com/aibasel/downward>

## Scientific experiments

We recommend to use the [latest release](https://github.com/aibasel/downward/releases/latest) instead of the tip of the main branch.
The [Downward Lab](https://lab.readthedocs.io/en/stable/) Python package helps running Fast Downward experiments.
Our separate [benchmark repository](https://github.com/aibasel/downward-benchmarks) contains a collection of planning tasks.

## Supported software versions

The planner is mainly developed under Linux; and all of its features should work with no restrictions under this platform.
The planner should compile and run correctly on macOS, but we cannot guarantee that it works as well as under Linux.
The same comment applies for Windows, where additionally some diagnostic features (e.g., reporting peak memory usage when the planner is terminated by a signal) are not supported.
Setting time and memory limits and running portfolios is not supported under Windows either.

This version of Fast Downward has been tested with the following software versions:

| OS           | Python | C++ compiler                                                     | CMake |
| ------------ | ------ | ---------------------------------------------------------------- | ----- |
| Ubuntu 24.04 | 3.10   | GCC 14, Clang 18                                                 | 3.30  |
| Ubuntu 22.04 | 3.10   | GCC 12, Clang 15                                                 | 3.30  |
| macOS 14     | 3.10   | AppleClang 15                                                    | 3.30  |
| macOS 13     | 3.10   | AppleClang 15                                                    | 3.30  |
| Windows 10   | 3.8    | Visual Studio Enterprise 2019 (MSVC 19.29) and 2022 (MSVC 19.41) | 3.30  |

We test LP support with CPLEX 22.1.1 and SoPlex 7.1.1. On Ubuntu we
test both CPLEX and SoPlex. On Windows we currently only test CPLEX,
and on macOS we do not test LP solvers (yet).

## Build instructions

See [BUILD.md](BUILD.md).


## Contributors

The following list includes all people that actively contributed to
Fast Downward, i.e., all people that appear in some commits in Fast
Downward's history (see below for a history on how Fast Downward
emerged) or people that influenced the development of such commits.
Currently, this list is sorted by the last year the person has been
active, and in case of ties, by the earliest year the person started
contributing, and finally by last name.

- 2003-2024 Malte Helmert
- 2008-2016, 2018-2024 Gabriele Roeger
- 2010-2024 Jendrik Seipp
- 2010-2011, 2013-2024 Silvan Sievers
- 2012-2024 Florian Pommerening
- 2013, 2015-2024 Salomé Eriksson
- 2018-2024 Patrick Ferber
- 2021-2024 Clemens Büchner
- 2022-2024 Remo Christen
- 2023-2024 Simon Dold
- 2023-2024 Claudia S. Grundke
- 2024 Martín Pozo
- 2024 Tanja Schindler
- 2024 David Speck
- 2015, 2021-2023 Thomas Keller
- 2018-2020, 2023 Augusto B. Corrêa
- 2023 Victor Paléologue
- 2023 Emanuele Tirendi
- 2021-2022 Dominik Drexler
- 2016-2020 Cedric Geissmann
- 2017-2020 Guillem Francès
- 2020 Rik de Graaff
- 2015-2019 Manuel Heusner
- 2017 Daniel Killenberger
- 2016 Yusra Alkhazraji
- 2016 Martin Wehrle
- 2014-2015 Patrick von Reth
- 2009-2014 Erez Karpas
- 2014 Robert P. Goldman
- 2010-2012 Andrew Coles
- 2010, 2012 Patrik Haslum
- 2003-2011 Silvia Richter
- 2009-2011 Emil Keyder
- 2010-2011 Moritz Gronbach
- 2010-2011 Manuela Ortlieb
- 2011 Vidal Alcázar Saiz
- 2011 Michael Katz
- 2011 Raz Nissim
- 2010 Moritz Goebelbecker
- 2007-2009 Matthias Westphal
- 2009 Christian Muise


## History

The current version of Fast Downward is the merger of three different
projects:

- the original version of Fast Downward developed by Malte Helmert
  and Silvia Richter
- LAMA, developed by Silvia Richter and Matthias Westphal based on
  the original Fast Downward
- FD-Tech, a modified version of Fast Downward developed by Erez
  Karpas and Michael Katz based on the original code

In addition to these three main sources, the codebase incorporates
code and features from numerous branches of the Fast Downward codebase
developed for various research papers. The main contributors to these
branches are Malte Helmert, Gabi Röger and Silvia Richter.


## License

```
Fast Downward is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

Fast Downward is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
```
