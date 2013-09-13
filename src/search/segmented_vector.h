#ifndef SEGMENTED_VECTOR_H
#define SEGMENTED_VECTOR_H

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>

#include "utilities.h"

// TODO: Get rid of the code duplication here. How to do it without
// paying a performance penalty?

template<class Entry, class Allocator = std::allocator<Entry> >
class SegmentedVector {
    typedef typename Allocator::template rebind<Entry>::other EntryAllocator;
    // TODO: Try to find a good value for SEGMENT_BYTES.
    static const size_t SEGMENT_BYTES = 8192;

    static const size_t SEGMENT_ELEMENTS =
        (SEGMENT_BYTES / sizeof(Entry)) >= 1 ?
        (SEGMENT_BYTES / sizeof(Entry)) : 1;

    EntryAllocator entry_allocator;

    std::vector<Entry *> segments;
    size_t the_size;

    size_t get_segment(size_t index) const {
        return index / SEGMENT_ELEMENTS;
    }

    size_t get_offset(size_t index) const {
        return index % SEGMENT_ELEMENTS;
    }

    void add_segment() {
        Entry *new_segment = entry_allocator.allocate(SEGMENT_ELEMENTS);
        segments.push_back(new_segment);
    }
public:
    SegmentedVector()
        : the_size(0) {
    }

    SegmentedVector(const EntryAllocator &allocator_)
        : entry_allocator(allocator_),
          the_size(0) {
    }

    ~SegmentedVector() {
        for (size_t i = 0; i < segments.size(); ++i)
            entry_allocator.deallocate(segments[i], SEGMENT_ELEMENTS);
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
            add_segment();
        }
        segments[segment][offset] = entry;
        ++the_size;
    }

    void resize(size_t new_size, Entry entry = Entry()) {
        if (new_size < the_size) {
            std::cerr << "SegmentedVector does not support removing elements. "
                      << "See revision 6ee5ff7b8873 for an implementation."
                      << std::endl;
            exit_with(EXIT_UNSUPPORTED);
        }
        while (new_size > the_size) {
            push_back(entry);
        }
    }
};


template<class Entry, class Allocator = std::allocator<Entry> >
class SegmentedArrayVector {
    typedef typename Allocator::template rebind<Entry>::other EntryAllocator;
    // TODO: Try to find a good value for SEGMENT_BYTES.
    static const size_t SEGMENT_BYTES = 8192;

    const size_t elements_per_array;
    const size_t arrays_per_segment;
    const size_t elements_per_segment;

    EntryAllocator entry_allocator;

    std::vector<Entry *> segments;
    size_t the_size;

    size_t get_segment(size_t index) const {
        return index / arrays_per_segment;
    }

    size_t get_offset(size_t index) const {
        return (index % arrays_per_segment) * elements_per_array;
    }

    void add_segment() {
        Entry *new_segment = entry_allocator.allocate(elements_per_segment);
        segments.push_back(new_segment);
    }
public:
    SegmentedArrayVector(size_t elements_per_array_)
        : elements_per_array(elements_per_array_),
          arrays_per_segment(
              std::max(SEGMENT_BYTES / (elements_per_array * sizeof(Entry)), size_t(1))),
          elements_per_segment(elements_per_array * arrays_per_segment),
          the_size(0) {
    }


    SegmentedArrayVector(size_t elements_per_array_, const EntryAllocator &allocator_)
        : entry_allocator(allocator_),
          elements_per_array(elements_per_array_),
          arrays_per_segment(
              std::max(SEGMENT_BYTES / (elements_per_array * sizeof(Entry)), size_t(1))),
          elements_per_segment(elements_per_array * arrays_per_segment),
          the_size(0) {
    }

    ~SegmentedArrayVector() {
        for (size_t i = 0; i < segments.size(); ++i)
            entry_allocator.deallocate(segments[i], elements_per_segment);
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
            add_segment();
        }
        Entry *dest = segments[segment] + offset;
        for (size_t i = 0; i < elements_per_array; ++i)
            *dest++ = *entry++;
        ++the_size;
    }

    void resize(size_t new_size, const Entry *entry) {
        if (new_size < the_size) {
            std::cerr << "SegmentedArrayVector does not support removing elements."
                      << std::endl;
            exit_with(EXIT_UNSUPPORTED);
        }
        while (new_size > the_size) {
            push_back(entry);
        }
    }
};

#endif
