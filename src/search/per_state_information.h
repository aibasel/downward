#ifndef PER_STATE_INFORMATION_H
#define PER_STATE_INFORMATION_H

#include "state_registry.h"

#include "algorithms/segmented_vector.h"
#include "algorithms/subscriber.h"
#include "utils/collections.h"

#include <cassert>
#include <iostream>
#include <unordered_map>

/*
  PerStateInformation is used to associate information with states.
  PerStateInformation<Entry> logically behaves somewhat like an unordered map
  from states to objects of class Entry. However, lookup of unknown states is
  supported and leads to insertion of a default value (similar to the
  defaultdict class in Python).

  For example, search algorithms can use it to associate g values or create
  operators with a state.

  Implementation notes: PerStateInformation is essentially implemented as a
  kind of two-level map:
    1. Find the correct SegmentedVector for the registry of the given state.
    2. Look up the associated entry in the SegmentedVector based on the ID of
       the state.
  It is common in many use cases that we look up information for states from
  the same registry in sequence. Therefore, to make step 1. more efficient, we
  remember (in "cached_registry" and "cached_entries") the results of the
  previous lookup and reuse it on consecutive lookups for the same registry.

  A PerStateInformation object subscribes to every StateRegistry for which it
  stores information. Once a StateRegistry is destroyed, it notifies all
  subscribed objects, which in turn destroy all information stored for states
  in that registry.
*/
template<class Entry>
class PerStateInformation : public subscriber::Subscriber<StateRegistry> {
    const Entry default_value;
    using EntryVectorMap = std::unordered_map<const StateRegistry *,
                                              segmented_vector::SegmentedVector<Entry> * >;
    EntryVectorMap entries_by_registry;

    mutable const StateRegistry *cached_registry;
    mutable segmented_vector::SegmentedVector<Entry> *cached_entries;

    /*
      Returns the SegmentedVector associated with the given StateRegistry.
      If no vector is associated with this registry yet, an empty one is created.
      Both the registry and the returned vector are cached to speed up
      consecutive calls with the same registry.
    */
    segmented_vector::SegmentedVector<Entry> *get_entries(const StateRegistry *registry) {
        if (cached_registry != registry) {
            cached_registry = registry;
            auto it = entries_by_registry.find(registry);
            if (it == entries_by_registry.end()) {
                cached_entries = new segmented_vector::SegmentedVector<Entry>();
                entries_by_registry[registry] = cached_entries;
                registry->subscribe(this);
            } else {
                cached_entries = it->second;
            }
        }
        assert(cached_registry == registry && cached_entries == entries_by_registry[registry]);
        return cached_entries;
    }

    /*
      Returns the SegmentedVector associated with the given StateRegistry.
      Returns nullptr, if no vector is associated with this registry yet.
      Otherwise, both the registry and the returned vector are cached to speed
      up consecutive calls with the same registry.
    */
    const segmented_vector::SegmentedVector<Entry> *get_entries(const StateRegistry *registry) const {
        if (cached_registry != registry) {
            const auto it = entries_by_registry.find(registry);
            if (it == entries_by_registry.end()) {
                return nullptr;
            } else {
                cached_registry = registry;
                cached_entries = const_cast<segmented_vector::SegmentedVector<Entry> *>(it->second);
            }
        }
        assert(cached_registry == registry);
        return cached_entries;
    }

public:
    PerStateInformation()
        : default_value(),
          cached_registry(nullptr),
          cached_entries(nullptr) {
    }

    explicit PerStateInformation(const Entry &default_value_)
        : default_value(default_value_),
          cached_registry(nullptr),
          cached_entries(nullptr) {
    }

    PerStateInformation(const PerStateInformation<Entry> &) = delete;
    PerStateInformation &operator=(const PerStateInformation<Entry> &) = delete;

    virtual ~PerStateInformation() override {
        for (auto it : entries_by_registry) {
            delete it.second;
        }
    }

    Entry &operator[](const State &state) {
        const StateRegistry *registry = state.get_registry();
        if (!registry) {
            std::cerr << "Tried to access per-state information with an "
                      << "unregistered state." << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        segmented_vector::SegmentedVector<Entry> *entries = get_entries(registry);
        int state_id = state.get_id().value;
        assert(state.get_id() != StateID::no_state);
        size_t virtual_size = registry->size();
        assert(utils::in_bounds(state_id, *registry));
        if (entries->size() < virtual_size) {
            entries->resize(virtual_size, default_value);
        }
        return (*entries)[state_id];
    }

    const Entry &operator[](const State &state) const {
        const StateRegistry *registry = state.get_registry();
        if (!registry) {
            std::cerr << "Tried to access per-state information with an "
                      << "unregistered state." << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        const segmented_vector::SegmentedVector<Entry> *entries = get_entries(registry);
        if (!entries) {
            return default_value;
        }
        int state_id = state.get_id().value;
        assert(state.get_id() != StateID::no_state);
        assert(utils::in_bounds(state_id, *registry));
        int num_entries = entries->size();
        if (state_id >= num_entries) {
            return default_value;
        }
        return (*entries)[state_id];
    }

    virtual void notify_service_destroyed(const StateRegistry *registry) override {
        delete entries_by_registry[registry];
        entries_by_registry.erase(registry);
        if (registry == cached_registry) {
            cached_registry = nullptr;
            cached_entries = nullptr;
        }
    }
};

#endif
