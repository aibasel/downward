#ifndef SEGMENTED_VECTOR_H
#define SEGMENTED_VECTOR_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

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

    Entry *add_segment() {
        Entry *new_segment = entry_allocator.allocate(SEGMENT_ELEMENTS);
        segments.push_back(new_segment);
        return new_segment;
    }

    void fill(Entry *position, size_t n, const Entry &entry) {
        std::fill(position, position + ptrdiff_t(n), entry);
    }

public:
    SegmentedVector()
        : entry_allocator(),
          the_size(0) {
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
        if (new_size > the_size) {
            // 1) Try to fill current segment
            size_t offset = get_offset(the_size);
            if (offset > 0) {
                size_t available = SEGMENT_ELEMENTS - offset;
                size_t new_entries = std::min(new_size - the_size, available);
                fill(&segments.back()[offset], new_entries, entry);
                the_size += new_entries;
            }
            // 2) Add full segments
            while (new_size - the_size >= SEGMENT_ELEMENTS) {
                Entry *new_segment = add_segment();
                fill(new_segment, SEGMENT_ELEMENTS, entry);
                the_size += SEGMENT_ELEMENTS;
            }
            // 3) Add partially filled segments
            if (new_size > the_size) {
                Entry *new_segment = add_segment();
                fill(new_segment, new_size - the_size, entry);
                the_size = new_size;
            }
        } else if (new_size < the_size) {
            // 1) Remove from current segment
            size_t offset = get_offset(the_size);
            size_t n_remove = std::min(the_size - new_size, offset);
            size_t remove_from_index = offset - n_remove;
            entry_allocator.deallocate(&segments.back()[remove_from_index], n_remove);
            the_size -= n_remove;
            // 2) Remove full segments
            size_t n_remove_segments = (the_size - new_size) / SEGMENT_ELEMENTS;
            for (size_t i = segments.size() - n_remove_segments; i < segments.size(); ++i) {
                entry_allocator.deallocate(segments[i], SEGMENT_ELEMENTS);
            }
            segments.resize(segments.size() - n_remove_segments);
            the_size -= SEGMENT_ELEMENTS * n_remove_segments;
            // 3) Remove from current segment
            n_remove = the_size - new_size;
            remove_from_index = SEGMENT_ELEMENTS - n_remove;
            entry_allocator.deallocate(&segments.back()[remove_from_index], n_remove);
            the_size = new_size;
        }
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
