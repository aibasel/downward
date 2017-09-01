#include <algorithm>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>

#include "hash.h"

using namespace std;


static void benchmark(const string &desc, int num_calls,
                      const function<void()> &func) {
    cout << "Running " << desc << " " << num_calls << " times:" << flush;

    clock_t start = clock();
    for (int j = 0; j < num_calls; ++j)
        func();
    clock_t end = clock();
    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    cout << " " << duration << "s" << endl;
}


static int scramble(int i) {
    return (0xdeadbeef * i) ^ 0xfeedcafe;
}


int main(int, char **) {
    const int REPETITIONS = 2;
    const int NUM_CALLS = 100000;
    const int NUM_INSERTIONS = 100;

    for (int i = 0; i < REPETITIONS; ++i) {
        benchmark("nothing", NUM_CALLS, [] () {});
        cout << endl;

        char *p = 0;
        unordered_set<pair<void *, void *>> boost_set;
        utils::HashSet<pair<void *, void *>> burtle_set;
        for (int i = 0; i < NUM_INSERTIONS; ++i) {
            auto pair = make_pair(p + scramble(i), p + scramble(i + 1));
            boost_set.insert(pair);
            burtle_set.insert(pair);
        }

        benchmark("insert pair<void *, void *> with BoostHash", NUM_CALLS,
                  [&]() {
            char *p = 0;
            unordered_set<pair<void *, void *>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(p + scramble(i), p + scramble(i + 1)));
            }
        });
        benchmark("insert pair<void *, void *> with BurtleFeed", NUM_CALLS,
                  [&]() {
            char *p = 0;
            utils::HashSet<pair<void *, void *>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(p + scramble(i), p + scramble(i + 1)));
            }
        });
        cout << endl;

        benchmark("count pair<void *, void *> with BoostHash", NUM_CALLS,
                  [&]() {
            char *p = 0;
            // Find existing key.
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                boost_set.count(make_pair(p + scramble(i), p + scramble(i + 1)));
            }
            // Find missing key.
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                boost_set.count(make_pair(p + scramble(i) + 1, p + scramble(i + 1)));
            }
        });
        benchmark("count pair<void *, void *> with BurtleFeed", NUM_CALLS,
                  [&]() {
            char *p = 0;
            // Find existing key.
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                burtle_set.count(make_pair(p + scramble(i), p + scramble(i + 1)));
            }
            // Find missing key.
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                burtle_set.count(make_pair(p + scramble(i) + 1, p + scramble(i + 1)));
            }
        });
        cout << endl;

        benchmark("insert sequential int with BoostHash", NUM_CALLS,
                  [&]() {
            unordered_set<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(i);
            }
        });
        benchmark("insert sequential int with BurtleFeed", NUM_CALLS,
                  [&]() {
            utils::HashSet<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(i);
            }
        });
        cout << endl;

        benchmark("insert scrambled int with BoostHash", NUM_CALLS,
                  [&]() {
            unordered_set<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(scramble(i));
            }
        });
        benchmark("insert scrambled int with BurtleFeed", NUM_CALLS,
                  [&]() {
            utils::HashSet<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(scramble(i));
            }
        });
        cout << endl;
    }

    return 0;
}
