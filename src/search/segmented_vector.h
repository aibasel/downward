#ifndef SEGMENTED_VECTOR_H
#define SEGMENTED_VECTOR_H

#include <algorithm>
#include <cassert>
#include <vector>


/*
  SegmentedVector is a vector-like class with the following advantages over
  vector:
    1. Resizing has no memory spike. (*)
    2. Should work more nicely with fragmented memory because data is
       partitioned into fixed-size chunks of size SEGMENT_BYTES.
    3. Overallocation is only additive (by SEGMENT_BYTES), not multiplicative
       as in vector. (*)
    4. References stay stable forever, so there is no need to be careful about
       invalidating references upon growing the vector.

  (*) Assumes that the size of the "segments" vector can be neglected, which is
  true if SEGMENT_BYTES isn't chosen too small. For example, with 1 GB of data
  and SEGMENT_BYTES = 8192, we can have 131072 segments.

  The main disadvantage to vector is that there is an additional indirection
  for each lookup, but we hope that the first lookup will usually hit the cache.
  The implementation is basically identical to that of deque (at least the
  g++ version), but with the advantage that we can control SEGMENT_BYTES. A
  test on all optimal planning instances with several planner configurations
  showed a modest advantage over deque.

  The class can also be used as a simple "memory pool" to reduce allocation
  costs (time and memory) when allocating many objects of the same type.

  SegmentedArrayVector is a similar class that can be used for compactly
  storing many fixed-size arrays. It's essentially a variant of SegmentedVector
  where the size of the stored data is only known at runtime, not at compile
  time.
*/

// TODO: Get rid of the code duplication here. How to do it without
// paying a performance penalty? issue388.

// For documentation on classes relevant to storing and working with registered
// states see the file state_registry.h.

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
            // constructor, like vector does: issue387.
            Entry *new_segment = new Entry[SEGMENT_ELEMENTS];
            segments.push_back(new_segment);
        }
        segments[segment][offset] = entry;
        ++the_size;
    }
};


template<class Element>
class SegmentedArrayVector {
    // TODO: Try to find a good value for SEGMENT_BYTES.
    static const size_t SEGMENT_BYTES = 8192;

    const size_t elements_per_array;
    const size_t arrays_per_segment;
    const size_t elements_per_segment;

    std::vector<Element *> segments;
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
              std::max(SEGMENT_BYTES / (elements_per_array * sizeof(Element)),
                       size_t(1))),
          elements_per_segment(elements_per_array * arrays_per_segment),
          the_size(0) {
    }

    ~SegmentedArrayVector() {
        for (size_t i = 0; i < segments.size(); ++i)
            delete[] segments[i];
    }

    Element *operator[](size_t index) {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment] + offset;
    }

    const Element *operator[](size_t index) const {
        assert(index < the_size);
        size_t segment = get_segment(index);
        size_t offset = get_offset(index);
        return segments[segment] + offset;
    }

    size_t size() const {
        return the_size;
    }

    void push_back(const Element *entry) {
        size_t segment = get_segment(the_size);
        size_t offset = get_offset(the_size);
        if (offset == 0) {
            assert(segment == segments.size());
            // Must add a new segment.
            Element *new_segment = new Element[elements_per_segment];
            segments.push_back(new_segment);
        }
        Element *dest = segments[segment] + offset;
        for (size_t i = 0; i < elements_per_array; ++i)
            *dest++ = *entry++;
        ++the_size;
    }
};

#endif
