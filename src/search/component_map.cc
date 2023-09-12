#include "component_map.h"
#include "utils/system.h"

using namespace std;

shared_ptr<unordered_map<void*, plugins::Any>> ComponentMap::get_inner_map(std::shared_ptr<AbstractTask> key){
    if (auto search = component_map->find(key); search != component_map->end()){
        return search->second;
    }
    else {
        cerr << "ComponentMap does not contain key." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}


bool ComponentMap::contains_key(KEY key){
    if (!component_map->contains(key.first)){
        return false;
    } else {
        auto inner_map = component_map->at(key.first);
        return inner_map->contains(key.second);
    }
}


VALUE ComponentMap::get_value(KEY key) {
    shared_ptr<unordered_map<void*, plugins::Any>> inner_map = get_inner_map(key.first);

    if (auto search = inner_map->find(key.second); search != inner_map->end()){
        return search->second;
    }
    else {
        cerr << "ComponentMap does not contain key." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}


void ComponentMap::add_entry(KEY key, VALUE value) {
    if (!component_map->contains(key.first)){
        auto inner_map = make_shared<
                                    unordered_map<
                                            void*, plugins::Any
                                                                 >
                                                 >
                                  (unordered_map<void*, plugins::Any>{{key.second, value}});
        component_map ->insert(make_pair(key.first, inner_map));
    }
    auto inner_map = get_inner_map(key.first);
    inner_map ->insert(std::make_pair(key.second, value));
}
