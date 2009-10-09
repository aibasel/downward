#include <cassert>
#include <iostream>
using namespace std;

#include "search_engine.h"
#include "timer.h"

SearchEngine::SearchEngine() {
    solved = false;
}

SearchEngine::~SearchEngine() {
}

void SearchEngine::statistics() const {
}

bool SearchEngine::found_solution() const {
    return solved;
}

const SearchEngine::Plan &SearchEngine::get_plan() const {
    assert(solved);
    return plan;
}

void SearchEngine::set_plan(const Plan &p) {
    solved = true;
    plan = p;
}

void SearchEngine::search() {
    initialize();
    Timer timer;
    while(step() == IN_PROGRESS)
	;
    cout << "Actual search time: " << timer << endl;
}

