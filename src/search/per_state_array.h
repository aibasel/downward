#ifndef PER_STATE_ARRAY_H
#define PER_STATE_ARRAY_H

#include "per_state_information.h"

#include <cassert>
#include <unordered_map>


template<class T>
class ConstArrayView {
    const T *p;
    int size_;
public:
    ConstArrayView(const T *p, int size) : p(p), size_(size) {}
    ConstArrayView(const ConstArrayView<T> &other) = default;

    ConstArrayView<T> &operator=(const ConstArrayView<T> &other) = default;

    const T &operator[](int index) const {
        assert(index >= 0 && index < size_);
        return p[index];
    }

    int size() const {
        return size_;
    }
};

template<class T>
class ArrayView {
    T *p;
    int size_;
public:
    ArrayView(T *p, int size) : p(p), size_(size) {}
    ArrayView(const ArrayView<T> &other) = default;

    ArrayView<T> &operator=(const ArrayView<T> &other) = default;

    operator ConstArrayView<T>() const {
        return ConstArrayView<T>(p, size_);
    }

    T &operator[](int index) {
        assert(index >= 0 && index < size_);
        return p[index];
    }

    const T &operator[](int index) const {
        assert(index >= 0 && index < size_);
        return p[index];
    }

    int size() const {
        return size_;
    }
};

/*
  PerStateArray is used to associate array-like information with states.
  PerStateArray<Entry> logically behaves somewhat like an unordered map
  from states to equal-length arrays of class Entry. However, lookup of
  unknown states is supported and leads to insertion of a default value
  (similar to the defaultdict class in Python).

  The implementation is similar to the one of PerStateInformation, which
  also contains more documentation.
*/

template<class Element>
class PerStateArray : public subscriber::Subscriber<StateRegistry> {
    const std::vector<Element> default_array;
    using EntryArrayVectorMap = std::unordered_map<const StateRegistry *,
                                                   segmented_vector::SegmentedArrayVector<Element> *>;
    EntryArrayVectorMap entry_arrays_by_registry;

    mutable const StateRegistry *cached_registry;
    mutable segmented_vector::SegmentedArrayVector<Element> *cached_entries;

    segmented_vector::SegmentedArrayVector<Element> *get_entries(const StateRegistry *registry) {
        if (cached_registry != registry) {
            cached_registry = registry;
            auto it = entry_arrays_by_registry.find(registry);
            if (it == entry_arrays_by_registry.end()) {
                cached_entries = new segmented_vector::SegmentedArrayVector<Element>(
                    default_array.size());
                entry_arrays_by_registry[registry] = cached_entries;
                registry->subscribe(this);
            } else {
                cached_entries = it->second;
            }
        }
        assert(cached_registry == registry && cached_entries == entry_arrays_by_registry[registry]);
        return cached_entries;
    }

    const segmented_vector::SegmentedArrayVector<Element> *get_entries(
        const StateRegistry *registry) const {
        if (cached_registry != registry) {
            const auto it = entry_arrays_by_registry.find(registry);
            if (it == entry_arrays_by_registry.end()) {
                return nullptr;
            } else {
                cached_registry = registry;
                cached_entries = const_cast<segmented_vector::SegmentedArrayVector<Element> *>(
                    it->second);
            }
        }
        assert(cached_registry == registry);
        return cached_entries;
    }

public:
    explicit PerStateArray(const std::vector<Element> &default_array)
        : default_array(default_array),
          cached_registry(nullptr),
          cached_entries(nullptr) {
    }

    PerStateArray(const PerStateArray<Element> &) = delete;
    PerStateArray &operator=(const PerStateArray<Element> &) = delete;

    virtual ~PerStateArray() override {
        for (auto it : entry_arrays_by_registry) {
            delete it.second;
        }
    }

    ArrayView<Element> operator[](const State &state) {
        const StateRegistry *registry = state.get_registry();
        if (!registry) {
            std::cerr << "Tried to access per-state array with an unregistered "
                      << "state." << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        segmented_vector::SegmentedArrayVector<Element> *entries = get_entries(registry);
        int state_id = state.get_id().value;
        assert(state.get_id() != StateID::no_state);
        size_t virtual_size = registry->size();
        assert(utils::in_bounds(state_id, *registry));
        if (entries->size() < virtual_size) {
            entries->resize(virtual_size, default_array.data());
        }
        return ArrayView<Element>((*entries)[state_id], default_array.size());
    }

    ConstArrayView<Element> operator[](const State &state) const {
        const StateRegistry *registry = state.get_registry();
        if (!registry) {
            std::cerr << "Tried to access per-state array with an unregistered "
                      << "state." << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        const segmented_vector::SegmentedArrayVector<Element> *entries =
            get_entries(registry);
        if (!entries) {
            ABORT("PerStateArray::operator[] const tried to access "
                  "non-existing entry.");
        }
        int state_id = state.get_id().value;
        assert(state.get_id() != StateID::no_state);
        assert(utils::in_bounds(state_id, *registry));
        int num_entries = entries->size();
        if (state_id >= num_entries) {
            ABORT("PerStateArray::operator[] const tried to access "
                  "non-existing entry.");
        }
        return ConstArrayView<Element>((*entries)[state_id], default_array.size());
    }

    virtual void notify_service_destroyed(const StateRegistry *registry) override {
        delete entry_arrays_by_registry[registry];
        entry_arrays_by_registry.erase(registry);
        if (registry == cached_registry) {
            cached_registry = nullptr;
            cached_entries = nullptr;
        }
    }
};

#endif
