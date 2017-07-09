#include <algorithm>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_set>

#include "fast_hash.h"
#include "hash.h"
#include "SpookyV2.h"

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


#define rot32(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

#define mix32(a, b, c) \
    { \
        a -= c;  a ^= rot32(c, 4);  c += b; \
        b -= a;  b ^= rot32(a, 6);  a += c; \
        c -= b;  c ^= rot32(b, 8);  b += a; \
        a -= c;  a ^= rot32(c, 16);  c += b; \
        b -= a;  b ^= rot32(a, 19);  a += c; \
        c -= b;  c ^= rot32(b, 4);  b += a; \
    }

#define final32(a, b, c) \
    { \
        c ^= b; c -= rot32(b, 14); \
        a ^= c; a -= rot32(c, 11); \
        b ^= a; b -= rot32(a, 25); \
        c ^= b; c -= rot32(b, 16); \
        a ^= c; a -= rot32(c, 4);  \
        b ^= a; b -= rot32(a, 14); \
        c ^= b; c -= rot32(b, 24); \
    }

inline unsigned int hash_unsigned_int_sequence(
    const int *k, unsigned int length, unsigned int initval) {
    unsigned int a, b, c;

    // Set up the internal state.
    a = b = c = 0xdeadbeef + (length << 2) + initval;

    // Handle most of the key.
    while (length > 3) {
        a += k[0];
        b += k[1];
        c += k[2];
        mix32(a, b, c);
        length -= 3;
        k += 3;
    }

    // Handle the last 3 unsigned ints. All case statements fall through.
    switch (length) {
    case 3:
        c += k[2];
    case 2:
        b += k[1];
    case 1:
        a += k[0];
        final32(a, b, c);
    // case 0: nothing left to add.
    case 0:
        break;
    }

    return c;
}

struct BurtleBurtleHash {
    std::size_t operator()(const std::vector<int> &vec) const {
        return hash_unsigned_int_sequence(vec.data(), vec.size(), 2016);
    }
};

using BurtleBurtleHashSet = std::unordered_set<vector<int>, BurtleBurtleHash>;


struct HashWordHash {
    std::size_t operator()(const std::vector<int> &vec) const {
        utils::HashState hash_state;
        hash_state.feed_ints(vec.data(), vec.size());
        return hash_state.get_hash64();
    }
};


struct SpookyV2Hash {
    std::size_t operator()(const std::vector<int> &vec) const {
        return SpookyHash::Hash64(vec.data(), vec.size() * 4, 2016);
    }
};

struct SpookyV2HashInt {
    std::size_t operator()(int i) const {
        return SpookyHash::Hash64(&i, sizeof(int), 2016);
    }
};


int main(int, char **) {
    const int REPETITIONS = 2;
    const int NUM_CALLS = 100000;
    const int NUM_INSERTIONS = 100;

    for (int i = 0; i < REPETITIONS; ++i) {
        benchmark("nothing", NUM_CALLS, [] () {});
        cout << endl;
        benchmark("insert int with BoostHash", NUM_CALLS,
                  [&]() {
            unordered_set<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(scramble(i));
            }
        });
        benchmark("insert int with BoostHashFeed", NUM_CALLS,
                  [&]() {
            fast_hash::HashSet<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(scramble(i));
            }
        });
        benchmark("insert int with BurtleFeed", NUM_CALLS,
                  [&]() {
            utils::HashSet<int> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(scramble(i));
            }
        });
        benchmark("insert int with SpookyHash", NUM_CALLS,
                  [&]() {
            std::unordered_set<int, SpookyV2HashInt> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(scramble(i));
            }
        });
        cout << endl;

        benchmark("insert pair<int, int> with BoostHash", NUM_CALLS,
                  [&]() {
            unordered_set<pair<int, int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(scramble(i), scramble(i + 1)));
            }
        });
        benchmark("insert pair<int, int> with BoostHashFeed", NUM_CALLS,
                  [&]() {
            fast_hash::HashSet<pair<int, int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(scramble(i), scramble(i + 1)));
            }
        });
        benchmark("insert pair<int, int> with BurtleFeed", NUM_CALLS,
                  [&]() {
            utils::HashSet<pair<int, int>> s;
            for (int i = 0; i < NUM_INSERTIONS; ++i) {
                s.insert(make_pair(scramble(i), scramble(i + 1)));
            }
        });
        cout << endl;

        for (int length : {1, 10, 100}
             ) {
            benchmark(
                "insert vector<int> of size " + to_string(length) +
                " with BoostHash", NUM_CALLS,
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
                " with BoostHashFeed", NUM_CALLS,
                [&]() {
                fast_hash::HashSet<vector<int>> s;
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
                " with BurtleVector", NUM_CALLS,
                [&]() {
                BurtleBurtleHashSet s;
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
                " with BurtleFeed", NUM_CALLS,
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
            benchmark(
                "insert vector<int> of size " + to_string(length) +
                " with BurtleFeedVector", NUM_CALLS,
                [&]() {
                std::unordered_set<vector<int>, HashWordHash> s;
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
                " with SpookyHash", NUM_CALLS,
                [&]() {
                std::unordered_set<vector<int>, SpookyV2Hash> s;
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
