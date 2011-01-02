#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <cassert>
#include <iostream>
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

/* 
   TODO: Test performance impact of using virtual methods.
   This would clean up the AdaptiveQueue code.
*/

template<typename Value>
class BucketQueue {
    typedef std::vector<Value> Bucket;
    std::vector<Bucket> buckets;
    mutable size_t current_bucket_no;
    size_t num_elements;
    size_t num_pushes;

    void update_current_bucket_no() const {
        while (current_bucket_no < buckets.size() &&
               buckets[current_bucket_no].empty())
            ++current_bucket_no;
    }
public:
    typedef std::pair<int, Value> Entry;

    BucketQueue() : current_bucket_no(0), num_elements(0), num_pushes(0) {
    }

    ~BucketQueue() {
    }

    void push(int key, Value value) {
        ++num_elements;
        ++num_pushes;
        assert(num_pushes); // Check against overflow.
        if (key >= buckets.size())
            buckets.resize(key + 1);
        else if (key < current_bucket_no)
            current_bucket_no = key;
        buckets[key].push_back(value);
    }

    Entry pop() {
        assert(!empty());
        assert(num_elements);
        --num_elements;
        update_current_bucket_no();
        Bucket &current_bucket = buckets[current_bucket_no];
        Value top_element = current_bucket.back();
        current_bucket.pop_back();
        return std::make_pair(current_bucket_no, top_element);
    }

    size_t size() const {
        return num_elements;
    }

    bool empty() const {
        return num_elements == 0;
    }

    void clear() {
        /* Clear the buckets, but don't release their memory. This is
           helpful if we want to use this queue again with similar
           usage patterns and want to reduce dynamic memory time
           overhead. */

        for (size_t i = current_bucket_no; num_elements; ++i) {
            assert(i < buckets.size());
            assert(buckets[i].size() <= num_elements);
            num_elements -= buckets[i].size();
            buckets[i].clear();
        }
        current_bucket_no = 0;
        assert(num_elements == 0);
        num_pushes = 0;
    }

    void clear_and_release_memory() {
        std::vector<Bucket> empty;
        buckets.swap(empty);
        current_bucket_no = 0;
        num_elements = 0;
        num_pushes = 0;
    }

    void get_sorted_entries(std::vector<Entry> &result) const {
        // Generate vector with entries of the queue in sorted order.
        result.clear();
        result.reserve(num_elements);
        size_t remaining_elements = num_elements;
        for (size_t key = current_bucket_no; remaining_elements; ++key) {
            const Bucket &bucket = buckets[key];
            for (size_t i = 0; i < bucket.size(); ++i)
                result.push_back(std::make_pair(key, bucket[i]));
            remaining_elements -= bucket.size();
        }
    }

    size_t get_num_pushes() const {
        // Return number of pushes since last call to clear().
        return num_pushes;
    }
};


template<typename Value>
class HeapQueue {
public:
    typedef std::pair<int, Value> Entry;
private:
    class compare_func {
    public:
        bool operator()(const Entry &lhs, const Entry &rhs) const {
            return lhs.first > rhs.first;
        }
    };

    class Heap : public std::priority_queue<
        Entry, std::vector<Entry>, compare_func> {
        // We inherit since our friend needs access to the underlying
        // container c which is a protected member.
        friend class HeapQueue;
    };

    Heap heap;
public:
    HeapQueue() {
    }

    ~HeapQueue() {
    }

    void push(int key, Value value) {
        heap.push(std::make_pair(key, value));
    }

    Entry pop() {
        assert(!heap.empty());
        Entry result = heap.top();
        heap.pop();
        return result;
    }

    size_t size() const {
        return heap.size();
    }

    bool empty() const {
        return heap.empty();
    }

    void clear() {
        heap.c.clear();
    }

    void clear_and_release_memory() {
        std::vector<Entry> empty;
        heap.c.swap(empty);
    }

    void set_sorted_entries_destructively(std::vector<Entry> &entries) {
        // Sets the entries of the heap to the given vector, which
        // must be sorted by key. The passed-in vector is cleared in
        // the process. May only be called on an empty heap.
        assert(empty());
        heap.c.swap(entries);
        // Since the entries are sorted, we do not need to heapify.
    }
};


/*
  AdaptiveQueue behaves like a BucketQueue initially, but changes into
  a HeapQueue once the ratio of elements to buckets becomes too small.
  Once turned into a HeapQueue, it stays a HeapQueue forever.

  More precisely, we switch from BucketQueue to HeapQueue whenever the
  number of required buckets exceeds BUCKET_THRESHOLD and the total
  number of elements pushed into the queue since the last clear() (or
  since initialization) is lower than the number of required buckets.
 */

template<typename Value>
class AdaptiveQueue {
    static const int BUCKET_THRESHOLD = 100;

    bool use_bucket_queue;
    BucketQueue<Value> bucket_queue;
    HeapQueue<Value> heap_queue;

public:
    typedef std::pair<int, Value> Entry;

    AdaptiveQueue() : use_bucket_queue(true) {
    }

    ~AdaptiveQueue() {
    }

    void push(int key, Value value) {
        if (use_bucket_queue) {
            if (key >= BUCKET_THRESHOLD &&
                bucket_queue.get_num_pushes() < key) {
                std::cout << "Adaptive queue switching from bucket-based "
                          << "to heap-based at key = " << key
                          << ", num_pushes = " << bucket_queue.get_num_pushes()
                          << std::endl;
                use_bucket_queue = false;
                std::vector<Entry> entries;
                bucket_queue.get_sorted_entries(entries);
                bucket_queue.clear_and_release_memory();
                heap_queue.set_sorted_entries_destructively(entries);
                heap_queue.push(key, value);
            }

            bucket_queue.push(key, value);
        } else {
            heap_queue.push(key, value);
        }
    }

    Entry pop() {
        if (use_bucket_queue)
            return bucket_queue.pop();
        else
            return heap_queue.pop();
    }

    size_t size() const {
        if (use_bucket_queue)
            return bucket_queue.size();
        else
            return heap_queue.size();
    }

    bool empty() const {
        if (use_bucket_queue)
            return bucket_queue.empty();
        else
            return heap_queue.empty();
    }

    void clear() {
        if (use_bucket_queue)
            bucket_queue.clear();
        else
            heap_queue.clear();
    }

    void clear_and_release_memory() {
        if (use_bucket_queue)
            bucket_queue.clear_and_release_memory();
        else
            heap_queue.clear_and_release_memory();
    }
};

#endif
