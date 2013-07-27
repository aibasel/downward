#ifndef SEGMENTED_VECTOR_H
#define SEGMENTED_VECTOR_H

#include <cassert>
#include <vector>

template<class Entry>
class SegmentedVector {
    // TODO: Try to find a good value for SEGMENT_BYTES.
    static const size_t SEGMENT_BYTES = 8192;

    static const size_t SEGMENT_ELEMENTS =
        (SEGMENT_BYTES / sizeof(Entry)) >= 1 ?
        (SEGMENT_BYTES / sizeof(Entry)) : 1;

    std::vector<Entry *> segments;
    size_t num_elements;

    size_t get_segment(size_t index) const {
        return index / SEGMENT_ELEMENTS;
    }

    size_t get_offset(size_t index) const {
        return index % SEGMENT_ELEMENTS;
    }
public:
    SegmentedVector()
        : num_elements(0) {
    }

    ~SegmentedVector() {
        for (size_t i = 0; i < segments.size(); ++i)
            delete[] segments[i];
    }

    Entry &operator[](size_t index) {
        assert(index < num_elements);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment][offset];
    }

    const Entry &operator[](size_t index) const {
        assert(index < num_elements);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment][offset];
    }

    size_t size() const {
        return num_elements;
    }

    void push_back(const Entry &entry) {
        size_t segment = get_segment(num_elements);
        size_t offset = get_offset(num_elements);
        if (offset == 0) {
            assert(segment == segments.size());
            // Must add a new segment.

            // TODO: Make this work for classes w/o a default
            // constructor, like vector does.
            Entry *new_segment = new Entry[SEGMENT_ELEMENTS];
            segments.push_back(new_segment);
        }
        segments[segment][offset] = entry;
        ++num_elements;
    }
};

#endif
