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
    const int NUM_CALLS = 1;
    const int NUM_INSERTIONS = 10000000;
    const int NUM_READ_PASSES = 10;

    for (int i = 0; i < REPETITIONS; ++i) {
        benchmark("nothing", NUM_CALLS, [] () {});
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

        benchmark("insert, then read sequential int with BoostHash", NUM_CALLS,
                  [&]() {
                      unordered_set<int> s;
                      for (int i = 0; i < NUM_INSERTIONS; ++i) {
                          s.insert(i);
                      }
                      for (int j = 0; j < NUM_READ_PASSES; ++j) {
                          for (int i = 0; i < NUM_INSERTIONS; ++i) {
                              s.count(i);
                          }
                      }
                  });
        benchmark("insert, then read sequential int with BurtleFeed", NUM_CALLS,
                  [&]() {
                      utils::HashSet<int> s;
                      for (int i = 0; i < NUM_INSERTIONS; ++i) {
                          s.insert(i);
                      }
                      for (int j = 0; j < NUM_READ_PASSES; ++j) {
                          for (int i = 0; i < NUM_INSERTIONS; ++i) {
                              s.count(i);
                          }
                      }
                  });
        cout << endl;

        benchmark("insert, then read scrambled int with BoostHash", NUM_CALLS,
                  [&]() {
                      unordered_set<int> s;
                      for (int i = 0; i < NUM_INSERTIONS; ++i) {
                          s.insert(scramble(i));
                      }
                      for (int j = 0; j < NUM_READ_PASSES; ++j) {
                          for (int i = 0; i < NUM_INSERTIONS; ++i) {
                              s.count(i);
                          }
                      }
                  });
        benchmark("insert, then read scrambled int with BurtleFeed", NUM_CALLS,
                  [&]() {
                      utils::HashSet<int> s;
                      for (int i = 0; i < NUM_INSERTIONS; ++i) {
                          s.insert(scramble(i));
                      }
                      for (int j = 0; j < NUM_READ_PASSES; ++j) {
                          for (int i = 0; i < NUM_INSERTIONS; ++i) {
                              s.count(i);
                          }
                      }
                  });
        cout << endl;
    }

    return 0;
}
