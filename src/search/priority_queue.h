#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <cassert>
#include <queue>
#include <utility>
#include <vector>


/*
  Implementation note: The various Queue classes have the same
  interface so that we can replace one class with another easily, but
  there is no inheritance relation since we don't currently need that
  and hence want to save the overhead that inheritance entails (virtual
  function calls + no inlining.)
 */


template<typename Value>
class BucketQueue {
    typedef std::vector<Value> Bucket;
    std::vector<Bucket> buckets;
    mutable size_t current_bucket_no;

    void update_current_bucket_no() const {
        while (current_bucket_no < buckets.size() &&
               buckets[current_bucket_no].empty())
            ++current_bucket_no;
    }
public:
    BucketQueue() : current_bucket_no(0) {
    }

    ~BucketQueue() {
    }

    void push(int key, Value value) {
        if (key >= buckets.size())
            buckets.resize(key + 1);
        else if (key < current_bucket_no)
            current_bucket_no = key;
        buckets[key].push_back(value);
    }

    std::pair<int, Value> pop() {
        update_current_bucket_no();
        assert(!empty());
        Bucket &current_bucket = buckets[current_bucket_no];
        Value top_element = current_bucket.back();
        current_bucket.pop_back();
        return std::make_pair(current_bucket_no, top_element);
    }

    bool empty() const {
        update_current_bucket_no();
        return current_bucket_no == buckets.size();
    }

    void clear() {
        /* Clear the buckets, but don't release their memory. This is
           helpful if we want to use this queue again with similar
           usage patterns and want to reduce dynamic memory time
           overhead. */

        for (size_t i = 0; i < buckets.size(); ++i)
            buckets[i].clear();
        current_bucket_no = 0;
    }

    void clear_and_release_memory() {
        buckets.swap(std::vector<Bucket>());
        current_bucket_no = 0;
    }
};

template<typename Value>
class HeapQueue {
    typedef std::pair<int, Value> Entry;
    class compare_func {
        // TODO: Test impact of using greater<Entry> instead (which
        //       would use the Value elements for tie-breaking).
    public:
        bool operator()(const Entry &lhs, const Entry &rhs) const {
            return lhs.first > rhs.first;
        }
    };
    typedef std::priority_queue<Entry, std::vector<Entry>, compare_func> Heap;
    Heap heap;
public:
    HeapQueue() {
    }

    ~HeapQueue() {
    }

    void push(int key, Value value) {
        heap.push(std::make_pair(key, value));
    }

    std::pair<int, Value> pop() {
        std::pair<int, Value> result = heap.top();
        heap.pop();
        return result;
    }

    bool empty() const {
        return heap.empty();
    }

    void clear() {
        heap.clear();
    }

    void clear_and_release_memory() {
        heap.swap(Heap());
    }
};


template<typename Value>
class AdaptiveQueue {
    // TODO: Implement
};

#endif
