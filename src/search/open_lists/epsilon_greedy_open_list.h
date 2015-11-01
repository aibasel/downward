#ifndef OPEN_LISTS_EPSILON_GREEDY_OPEN_LIST_H
#define OPEN_LISTS_EPSILON_GREEDY_OPEN_LIST_H

#include "open_list.h"

#include "../globals.h"
#include "../option_parser.h"
#include "../rng.h"

#include <functional>

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

template<class Entry>
class EpsilonGreedyOpenList : public OpenList<Entry> {
    class HeapNode {
public:
        int id;
        int h;
        Entry entry;
        HeapNode(int id, int h, const Entry &entry)
            : id(id), h(h), entry(entry) {
        }

        bool operator>(const HeapNode &other) const {
            return std::make_pair(h, id) > std::make_pair(other.h, other.id);
        }
    };

    std::vector<HeapNode> heap;
    ScalarEvaluator *evaluator;

    double epsilon;
    int size;
    int next_id;

protected:
    virtual void do_insertion(EvaluationContext &eval_context,
                              const Entry &entry) override {
        heap.emplace_back(
            next_id++, eval_context.get_heuristic_value(evaluator), entry);
        push_heap(heap.begin(), heap.end(), std::greater<HeapNode>());
        ++size;
    }

public:
    explicit EpsilonGreedyOpenList(const Options &opts)
        : OpenList<Entry>(opts.get<bool>("pref_only")),
          evaluator(opts.get<ScalarEvaluator *>("eval")),
          epsilon(opts.get<double>("epsilon")),
          size(0),
          next_id(0) {
    }

    virtual ~EpsilonGreedyOpenList() override = default;

    virtual Entry remove_min(std::vector<int> *key = 0) override {
        assert(size > 0);
        if (g_rng() < epsilon) {
            int pos = g_rng(size);
            heap[pos].h = std::numeric_limits<int>::min();
            adjust_heap_up(heap, pos);
        }
        pop_heap(heap.begin(), heap.end(), std::greater<HeapNode>());
        HeapNode heap_node = heap.back();
        heap.pop_back();
        if (key) {
            assert(key->empty());
            key->push_back(heap_node.h);
        }
        --size;
        return heap_node.entry;
    }

    virtual bool is_dead_end(
        EvaluationContext &eval_context) const override {
        return eval_context.is_heuristic_infinite(evaluator);
    }

    virtual bool is_reliable_dead_end(
        EvaluationContext &eval_context) const override {
        return is_dead_end(eval_context) && evaluator->dead_ends_are_reliable();
    }

    virtual void get_involved_heuristics(std::set<Heuristic *> &hset) override {
        evaluator->get_involved_heuristics(hset);
    }

    virtual bool empty() const override {
        return size == 0;
    }

    virtual void clear() override {
        heap.clear();
        size = 0;
        next_id = 0;
    }

    static void adjust_heap_up(std::vector<HeapNode> &heap, size_t pos) {
        assert(in_bounds(pos, heap));
        while (pos != 0) {
            size_t parent_pos = (pos - 1) / 2;
            if (heap[pos] > heap[parent_pos]) {
                break;
            }
            std::swap(heap[pos], heap[parent_pos]);
            pos = parent_pos;
        }
    }

    static OpenList<Entry> *_parse(OptionParser &parser) {
        parser.document_synopsis(
            "Epsilon-greedy open list",
            "Chooses an entry uniformly randomly with probability "
            "'epsilon', otherwise it returns the minimum entry. "
            "The algorithm is based on\n\n"
            " * Richard Valenzano, Nathan R. Sturtevant, "
            "Jonathan Schaeffer, and Fan Xie.<<BR>>\n"
            " [A Comparison of Knowledge-Based GBFS Enhancements and "
            "Knowledge-Free Exploration "
            "http://www.aaai.org/ocs/index.php/ICAPS/ICAPS14/paper/view/7943/8066]."
            "<<BR>>\n "
            "In //Proceedings of the Twenty-Fourth International "
            "Conference on Automated Planning and Scheduling (ICAPS "
            "2014)//, pp. 375-379. AAAI Press 2014.\n\n\n");
        parser.add_option<ScalarEvaluator *>("eval", "scalar evaluator");
        parser.add_option<bool>(
            "pref_only",
            "insert only nodes generated by preferred operators", "false");
        parser.add_option<double>(
            "epsilon",
            "probability for choosing the next entry randomly",
            "0.2",
            Bounds("0.0", "1.0"));
        Options opts = parser.parse();

        if (parser.dry_run()) {
            return nullptr;
        } else {
            return new EpsilonGreedyOpenList<Entry>(opts);
        }
    }
};

#endif
