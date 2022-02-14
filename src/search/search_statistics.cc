#include "search_statistics.h"

#include "utils/logging.h"
#include "utils/timer.h"
#include "utils/system.h"

#include <iostream>

using namespace std;


SearchStatistics::SearchStatistics(utils::LogProxy &log)
    : log(log) {
    expanded_states = 0;
    reopened_states = 0;
    evaluated_states = 0;
    evaluations = 0;
    generated_states = 0;
    dead_end_states = 0;
    generated_ops = 0;

    lastjump_expanded_states = 0;
    lastjump_reopened_states = 0;
    lastjump_evaluated_states = 0;
    lastjump_generated_states = 0;

    lastjump_f_value = -1;
}

void SearchStatistics::report_f_value_progress(int f) {
    if (f > lastjump_f_value) {
        lastjump_f_value = f;
        print_f_line();
        lastjump_expanded_states = expanded_states;
        lastjump_reopened_states = reopened_states;
        lastjump_evaluated_states = evaluated_states;
        lastjump_generated_states = generated_states;
    }
}

void SearchStatistics::print_f_line() const {
    if (log.is_at_least_normal()) {
        log << "f = " << lastjump_f_value
            << ", ";
        print_basic_statistics();
        log << endl;
    }
}

void SearchStatistics::print_checkpoint_line(int g) const {
    if (log.is_at_least_normal()) {
        log << "g=" << g << ", ";
        print_basic_statistics();
        log << endl;
    }
}

void SearchStatistics::print_basic_statistics() const {
    log << evaluated_states << " evaluated, "
        << expanded_states << " expanded";
    if (reopened_states > 0) {
        log << ", " << reopened_states << " reopened";
    }
}

void SearchStatistics::print_detailed_statistics() const {
    log << "Expanded " << expanded_states << " state(s)." << endl;
    log << "Reopened " << reopened_states << " state(s)." << endl;
    log << "Evaluated " << evaluated_states << " state(s)." << endl;
    log << "Evaluations: " << evaluations << endl;
    log << "Generated " << generated_states << " state(s)." << endl;
    log << "Dead ends: " << dead_end_states << " state(s)." << endl;

    if (lastjump_f_value >= 0) {
        log << "Expanded until last jump: "
            << lastjump_expanded_states << " state(s)." << endl;
        log << "Reopened until last jump: "
            << lastjump_reopened_states << " state(s)." << endl;
        log << "Evaluated until last jump: "
            << lastjump_evaluated_states << " state(s)." << endl;
        log << "Generated until last jump: "
            << lastjump_generated_states << " state(s)." << endl;
    }
}
