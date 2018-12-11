#! /usr/bin/env python
# -*- coding: utf-8 -*-
import common

# We want to test both NOT specifying the -sas-file option AND specifying the default "output.sas" value.
# The result should be the same in both cases
common.generate_experiments(common.generate_configs((None, "output.sas")))
