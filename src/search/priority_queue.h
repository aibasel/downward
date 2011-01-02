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
    size_t num_elements;

    void update_current_bucket_no() const {
        while (current_bucket_no < buckets.size() &&
               buckets[current_bucket_no].empty())
            ++current_bucket_no;
    }
public:
    BucketQueue() : current_bucket_no(0), num_elements(0) {
    }

    ~BucketQueue() {
    }

    void push(int key, Value value) {
        ++num_elements;
        if (key >= buckets.size())
            buckets.resize(key + 1);
        else if (key < current_bucket_no)
            current_bucket_no = key;
        buckets[key].push_back(value);
    }

    std::pair<int, Value> pop() {
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
    }

    void clear_and_release_memory() {
        buckets.swap(std::vector<Bucket>());
        current_bucket_no = 0;
        num_elements = 0;
    }
};


template<typename Value>
class HeapQueue {
    typedef std::pair<int, Value> Entry;

    class compare_func {
    public:
        bool operator()(const Entry &lhs, const Entry &rhs) const {
            return lhs.first > rhs.first;
        }
    };

    class Heap : public std::priority_queue<
        Entry, std::vector<Entry>, compare_func> {
        // We inherit since priority_queue doesn't have clear() or swap().
        // Inheriting gives us access to the protected element c, the
        // underlying container.
    public:
        void clear() {
            this->c.clear();
        }
        void clear_and_release_memory() {
            this->c.swap(std::vector<Entry>());
        }
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

    std::pair<int, Value> pop() {
        assert(!heap.empty());
        std::pair<int, Value> result = heap.top();
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
        heap.clear();
    }

    void clear_and_release_memory() {
        heap.clear_and_release_memory();
    }
};


/*
  AdaptiveQueue behaves like a BucketQueue initially, but changes into
  a HeapQueue once the ratio of elements to buckets becomes too small.
  Once turned into a HeapQueue, it stays a HeapQueue forever.
 */

template<typename Value>
class AdaptiveQueue {
    bool use_bucket_queue;
    BucketQueue<Value> bucket_queue;
    HeapQueue<Value> heap_queue;

    AdaptiveQueue() : use_bucket_queue(true) {
    }

    ~AdaptiveQueue() {
    }

    void push(int key, Value value) {
        if (use_bucket_queue) {
            // TODO: Switch representation if necessary.
            bucket_queue.push(key, value);
        } else {
            heap_queue.push(key, value);
        }
    }

    std::pair<int, Value> pop() {
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
