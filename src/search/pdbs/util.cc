#include "util.h"

#include "../utilities.h"
#include "../task_proxy.h"

#include <algorithm>
#include <iostream>

using namespace std;


void validate_and_normalize_pattern(const TaskProxy &task_proxy,
                                    Pattern &pattern) {
    /*
      - Sort by variable number and remove duplicate variables.
      - Warn if duplicate variables exist.
      - Error if patterns contain out-of-range variable numbers.
    */
    sort(pattern.begin(), pattern.end());
    auto it = unique(pattern.begin(), pattern.end());
    if (it != pattern.end()) {
        pattern.erase(it, pattern.end());
        cout << "Warning: duplicate variables in pattern have been removed"
             << endl;
    }
    if (!pattern.empty()) {
        if (pattern.front() < 0) {
            cerr << "Variable number too low in pattern" << endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
        int num_variables = task_proxy.get_variables().size();
        if (pattern.back() >= num_variables) {
            cerr << "Variable number too high in pattern" << endl;
            exit_with(EXIT_CRITICAL_ERROR);
        }
    }
}

void validate_and_normalize_patterns(const TaskProxy &task_proxy,
                                     Patterns &patterns) {
    /*
      - Validate and normalize each pattern (see there).
      - Sort collection lexicographically and remove duplicate patterns.
      - Warn if duplicate patterns exist.
    */
    for (auto &pattern : patterns)
        validate_and_normalize_pattern(task_proxy, pattern);
    sort(patterns.begin(), patterns.end());
    auto it = unique(patterns.begin(), patterns.end());
    if (it != patterns.end()) {
        patterns.erase(it, patterns.end());
        cout << "Warning: duplicate patterns have been removed" << endl;
    }
}
