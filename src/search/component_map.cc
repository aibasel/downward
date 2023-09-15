#include "component_map.h"

#include "utils/system.h"

using namespace std;

shared_ptr<Component> TaskIndependentComponent::create_task_specific_Component(
    [[maybe_unused]] shared_ptr<AbstractTask> &task,
    [[maybe_unused]] shared_ptr<ComponentMap> &component_map) {
    cerr << "Tries to create Component in an unimplemented way." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}

shared_ptr<Component> TaskIndependentComponent::create_task_specific_Component(shared_ptr<AbstractTask> &task) {
    std::shared_ptr<ComponentMap> component_map = std::make_shared<ComponentMap>();
    return create_task_specific_Component(task, component_map);
}


ComponentMap::ComponentMap() : component_map() {
}


shared_ptr<unordered_map<void *, plugins::Any>> ComponentMap::get_inner_map(std::shared_ptr<AbstractTask> key) {
    if (auto keyValuePair = component_map.find(key); keyValuePair != component_map.end()) {
        return keyValuePair->second;
    } else {
        cerr << "ComponentMap does not contain key." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}


bool ComponentMap::contains_key(KEY key) {
    bool b;
    if (!component_map.contains(key.first)) {
        b = false;
    } else {
        auto inner_map = component_map.at(key.first);
        b = inner_map->contains(key.second);
    }
    return b;
}


VALUE ComponentMap::get_value(KEY key) {
    shared_ptr<unordered_map<void *, plugins::Any>> inner_map = get_inner_map(key.first);

    if (auto search = inner_map->find(key.second); search != inner_map->end()) {
        return search->second;
    } else {
        cerr << "ComponentMap does not contain key." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}


void ComponentMap::add_entry(KEY key, VALUE value) {
    if (!component_map.contains(key.first)) {
        auto inner_map = make_shared<unordered_map<void *, plugins::Any>>(
            unordered_map<void *, plugins::Any>{{key.second, value}});
        component_map.insert(make_pair(key.first, inner_map));
    } else {
        auto inner_map = get_inner_map(key.first);
        inner_map->insert(make_pair(key.second, value));
    }
}

void ComponentMap::add_dual_key_entry(shared_ptr<AbstractTask> key1, TaskIndependentComponent *key2, VALUE value) {
    KEY pair = make_pair(key1, static_cast<void *>(key2));
    add_entry(pair, value);
}

VALUE ComponentMap::get_dual_key_value(std::shared_ptr<AbstractTask> key1, TaskIndependentComponent *key2) {
    KEY pair = make_pair(key1, static_cast<void *>(key2));
    return get_value(pair);
}
