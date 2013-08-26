#ifndef SEGMENTED_VECTOR_H
#define SEGMENTED_VECTOR_H

#include <algorithm>
#include <cassert>
#include <vector>

// TODO: Get rid of the code duplication here. How to do it without
// paying a performance penalty?

template<class Entry>
class SegmentedVector {
    // TODO: Try to find a good value for SEGMENT_BYTES.
    static const size_t SEGMENT_BYTES = 8192;

    static const size_t SEGMENT_ELEMENTS =
        (SEGMENT_BYTES / sizeof(Entry)) >= 1 ?
        (SEGMENT_BYTES / sizeof(Entry)) : 1;

    std::vector<Entry *> segments;
    size_t the_size;

    size_t get_segment(size_t index) const {
        return index / SEGMENT_ELEMENTS;
    }

    size_t get_offset(size_t index) const {
        return index % SEGMENT_ELEMENTS;
    }
public:
    SegmentedVector()
        : the_size(0) {
    }

    ~SegmentedVector() {
        for (size_t i = 0; i < segments.size(); ++i)
            delete[] segments[i];
    }

    Entry &operator[](size_t index) {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment][offset];
    }

    const Entry &operator[](size_t index) const {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment][offset];
    }

    size_t size() const {
        return the_size;
    }

    void push_back(const Entry &entry) {
        size_t segment = get_segment(the_size);
        size_t offset = get_offset(the_size);
        if (offset == 0) {
            assert(segment == segments.size());
            // Must add a new segment.

            // TODO: Make this work for classes w/o a default
            // constructor, like vector does.
            Entry *new_segment = new Entry[SEGMENT_ELEMENTS];
            segments.push_back(new_segment);
        }
        segments[segment][offset] = entry;
        ++the_size;
    }
};


template<class Entry>
class SegmentedArrayVector {
    // TODO: Try to find a good value for SEGMENT_BYTES.
    static const size_t SEGMENT_BYTES = 8192;

    const size_t elements_per_array;
    const size_t arrays_per_segment;
    const size_t elements_per_segment;

    std::vector<Entry *> segments;
    size_t the_size;

    size_t get_segment(size_t index) const {
        return index / arrays_per_segment;
    }

    size_t get_offset(size_t index) const {
        return (index % arrays_per_segment) * elements_per_array;
    }
public:
    SegmentedArrayVector(size_t elements_per_array_)
        : elements_per_array(elements_per_array_),
          arrays_per_segment(
              std::max(SEGMENT_BYTES / (elements_per_array * sizeof(Entry)),
                       size_t(1))),
          elements_per_segment(elements_per_array * arrays_per_segment),
          the_size(0) {
    }

    ~SegmentedArrayVector() {
        for (size_t i = 0; i < segments.size(); ++i)
            delete[] segments[i];
    }

    Entry *operator[](size_t index) {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment] + offset;
    }

    const Entry *operator[](size_t index) const {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment] + offset;
    }

    size_t size() const {
        return the_size;
    }

    void push_back(const Entry *entry) {
        size_t segment = get_segment(the_size);
        size_t offset = get_offset(the_size);
        if (offset == 0) {
            assert(segment == segments.size());
            // Must add a new segment.
            Entry *new_segment = new Entry[elements_per_segment];
            segments.push_back(new_segment);
        }
        Entry *dest = segments[segment] + offset;
        for (size_t i = 0; i < elements_per_array; ++i)
            *dest++ = *entry++;
        ++the_size;
    }
};

#endif
