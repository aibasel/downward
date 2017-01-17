#include <algorithm>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>

#include "hash.h"

using namespace std;


void benchmark(const string &desc, int num_calls,
               const function<void()> &func) {
    cout << "Running " << desc << " " << num_calls << " times:" << flush;
    clock_t start = clock();
    for (int i = 0; i < num_calls; ++i)
        func();
    clock_t end = clock();
    double duration = static_cast<double>(end - start) / CLOCKS_PER_SEC;
    cout << " " << duration << " seconds" << endl;
}


int main(int, char **) {
    const int NUM_ITERATIONS = 100000;
    const int NUM_INSERTIONS = 100;

    benchmark("nothing", NUM_ITERATIONS, [] () {});
    cout << endl;
    benchmark("insert int into std::unordered_set", NUM_ITERATIONS,
        [&]() {
            unordered_set<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(i);
            }
        });
    benchmark("insert int into utils::HashSet", NUM_ITERATIONS,
        [&]() {
            utils::HashSet<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(i);
            }
        });
    cout << endl;

    benchmark("insert pair<int, int> into std::unordered_set", NUM_ITERATIONS,
        [&]() {
            unordered_set<pair<int, int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(i, i + 1));
            }
        });
    benchmark("insert pair<int, int> into utils::HashSet", NUM_ITERATIONS,
        [&]() {
            utils::HashSet<pair<int, int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(i, i + 1));
            }
        });
    cout << endl;

    benchmark("insert vector<int> into std::unordered_set", NUM_ITERATIONS,
        [&]() {
            unordered_set<vector<int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                vector<int> v(i);
                iota(v.begin(), v.end(), 0);
                s.insert(v);
            }
        });
    benchmark("insert vector<int> into utils::HashSet", NUM_ITERATIONS,
        [&]() {
            utils::HashSet<vector<int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                vector<int> v(i);
                iota(v.begin(), v.end(), 0);
                s.insert(v);
            }
        });
    cout << endl;

    return 0;
}
