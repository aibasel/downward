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
    const int REPETITIONS = 3;
    const int NUM_CALLS = 100000;
    const int NUM_INSERTIONS = 100;

    for (int i = 0; i < REPETITIONS; ++i) {
        benchmark("nothing", NUM_CALLS, [] () {});
        cout << endl;
        benchmark("insert int into std::unordered_set", NUM_CALLS,
                  [&]() {
            unordered_set<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(scramble(i));
            }
        });
        benchmark("insert int into utils::HashSet", NUM_CALLS,
                  [&]() {
            utils::HashSet<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(scramble(i));
            }
        });
        cout << endl;

        benchmark("insert pair<int, int> into std::unordered_set", NUM_CALLS,
                  [&]() {
            unordered_set<pair<int, int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(scramble(i), scramble(i + 1)));
            }
        });
        benchmark("insert pair<int, int> into utils::HashSet", NUM_CALLS,
                  [&]() {
            utils::HashSet<pair<int, int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(scramble(i), scramble(i + 1)));
            }
        });
        cout << endl;

        for (int length : {1, 10, 100, 101}
             ) {
            benchmark(
                "insert vector<int> of size " + to_string(length) +
                " into std::unordered_set", NUM_CALLS,
                [&]() {
                unordered_set<vector<int>> s;
                for (int i = 0; i < NUM_INSERTIONS; ++i) {
                    vector<int> v;
                    v.reserve(length);
                    for (int j = 0; j < length; ++j) {
                        v.push_back(scramble(NUM_INSERTIONS * length + j));
                    }
                    s.insert(v);
                }
            });
            benchmark(
                "insert vector<int> of size " + to_string(length) +
                " into utils::HashSet", NUM_CALLS,
                [&]() {
                utils::HashSet<vector<int>> s;
                for (int i = 0; i < NUM_INSERTIONS; ++i) {
                    vector<int> v;
                    v.reserve(length);
                    for (int j = 0; j < length; ++j) {
                        v.push_back(scramble(NUM_INSERTIONS * length + j));
                    }
                    s.insert(v);
                }
            });
            cout << endl;
        }
        cout << endl;
    }

    return 0;
}
