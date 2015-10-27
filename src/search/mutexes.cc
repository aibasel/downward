#include "mutexes.h"

#include <vector>
#include <set>

using namespace std;


static vector<vector<set<pair<int, int>>>> inconsistent_facts;

bool facts_are_mutex(const pair<int, int> &a, const pair<int, int> &b) {
    if (a.first == b.first) {
        // Same variable: mutex iff different value.
        return a.second != b.second;
    }
    return bool(inconsistent_facts[a.first][a.second].count(b));
}
