#ifndef OPEN_LISTS_EPSILON_GREEDY_OPEN_LIST_H
#define OPEN_LISTS_EPSILON_GREEDY_OPEN_LIST_H

#include "../open_list_factory.h"

/*
    Epsilon-greedy open list based on Valenzano et al. (ICAPS 2014).

    With probability epsilon the next entry is selected uniformly
    randomly, otherwise the minimum entry is chosen. While the original
    implementation by Valenzano et al. is based on buckets (personal
    communication with the authors), this implementation stores entries
    in a heap. It is usually desirable to let open lists break ties in
    FIFO order. When using a heap, this can be achieved without using
    significantly more time by assigning increasing IDs to new entries
    and using the IDs as tiebreakers for entries with identical values.
    On the other hand, FIFO tiebreaking induces a substantial worst-case
    runtime penalty for bucket-based implementations. In the table below
    we list the worst-case time complexities for the discussed
    implementation strategies.

    n: number of entries
    m: number of buckets

                                Buckets       Buckets (no FIFO)    Heap
        Insert entry            O(log(m))     O(log(m))            O(log(n))
        Remove random entry     O(m + n)      O(m)                 O(log(n))
        Remove minimum entry    O(log(m))     O(log(m))            O(log(n))

    These results assume that the buckets are implemented as deques and
    are stored in a sorted dictionary, mapping from evaluator values to
    buckets. For inserting a new entry and removing the minimum entry the
    bucket-based implementations need to find the correct bucket
    (O(log(m))) and can then push or pop from one end of the deque
    (O(1)). For returning a random entry, bucket-based implementations
    need to loop over all buckets (O(m)) to find the one that contains
    the randomly selected entry. If FIFO ordering is ignored, one can use
    swap-and-pop to remove the entry in constant time. Otherwise, the
    removal is linear in the number of entries in the bucket (O(n), since
    there could be only one bucket).
*/

namespace epsilon_greedy_open_list {
class EpsilonGreedyOpenListFactory : public OpenListFactory {
    std::shared_ptr<Evaluator> eval;
    double epsilon;
    int random_seed;
    bool pref_only;
public:
    EpsilonGreedyOpenListFactory(
        const std::shared_ptr<Evaluator> &eval, double epsilon,
        int random_seed, bool pref_only);

    virtual std::unique_ptr<StateOpenList> create_state_open_list() override;
    virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
