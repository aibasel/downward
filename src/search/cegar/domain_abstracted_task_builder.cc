#include "domain_abstracted_task_builder.h"

#include <sstream>
#include <string>

using namespace std;

namespace cegar {
class DomainAbstractedTaskBuilder {
private:
    std::vector<int> domain_size;
    std::vector<int> initial_state_values;
    std::vector<Fact> goals;
    std::vector<std::vector<std::string> > fact_names;
    std::vector<std::vector<int> > value_map;
    std::shared_ptr<AbstractTask> task;

    void initialize(const std::shared_ptr<AbstractTask> parent);
    void move_fact(int var, int before, int after);
    void combine_values(int var, const ValueGroups &groups);
    std::string get_combined_fact_name(int var, const ValueGroup &values) const;

public:
    DomainAbstractedTaskBuilder(
        const std::shared_ptr<AbstractTask> parent,
        const VarToGroups &value_groups);
    ~DomainAbstractedTaskBuilder() = default;

    std::shared_ptr<AbstractTask> get_task() const;
};

DomainAbstractedTaskBuilder::DomainAbstractedTaskBuilder(
    const std::shared_ptr<AbstractTask> parent,
    const VarToGroups &value_groups) {
    initialize(parent);
    for (const auto &pair : value_groups) {
        int var = pair.first;
        const ValueGroups &groups = pair.second;
        combine_values(var, groups);
    }
    task = make_shared<DomainAbstractedTask>(
       parent, move(domain_size), move(initial_state_values), move(goals),
       move(fact_names), move(value_map));
}

void DomainAbstractedTaskBuilder::initialize(const shared_ptr<AbstractTask> parent) {
    int num_vars = parent->get_num_variables();
    domain_size.resize(num_vars);
    initial_state_values = parent->get_initial_state_values();
    value_map.resize(num_vars);
    fact_names.resize(num_vars);
    for (int var = 0; var < num_vars; ++var) {
        int num_values = parent->get_variable_domain_size(var);
        domain_size[var] = num_values;
        value_map[var].resize(num_values);
        fact_names[var].resize(num_values);
        for (int value = 0; value < num_values; ++value) {
            value_map[var][value] = value;
            fact_names[var][value] = parent->get_fact_name(var, value);
        }
    }
}

void DomainAbstractedTaskBuilder::move_fact(int var, int before, int after) {
    assert(in_bounds(var, domain_size));
    assert(0 <= before && before < domain_size[var]);
    assert(0 <= after && after < domain_size[var]);

    if (before == after)
        return;

    // Move each fact at most once.
    assert(value_map[var][before] == before);

    value_map[var][before] = after;
    fact_names[var][after] = move(fact_names[var][before]);
    if (initial_state_values[var] == before)
        initial_state_values[var] = after;
    for (Fact &goal : goals) {
        if (var == goal.first && before == goal.second)
            goal.second = after;
    }
}

string DomainAbstractedTaskBuilder::get_combined_fact_name(
    int var, const ValueGroup &values) const {
    stringstream name;
    string sep = "";
    for (int value : values) {
        name << sep << fact_names[var][value];
        sep = " OR ";
    }
    return name.str();
}

void DomainAbstractedTaskBuilder::combine_values(int var, const ValueGroups &groups) {
    vector<string> combined_fact_names;
    unordered_set<int> groups_union;
    int num_merged_values = 0;
    for (const ValueGroup &group : groups) {
        combined_fact_names.push_back(get_combined_fact_name(var, group));
        groups_union.insert(group.begin(), group.end());
        num_merged_values += group.size();
    }
    assert(static_cast<int>(groups_union.size()) == num_merged_values);

    int next_free_pos = 0;

    // Move all facts that are not part of groups to the front.
    for (int before = 0; before < domain_size[var]; ++before) {
        if (groups_union.count(before) == 0) {
            move_fact(var, before, next_free_pos++);
        }
    }
    int num_single_values = next_free_pos;
    assert(num_single_values + num_merged_values == domain_size[var]);

    // Add new facts for merged groups.
    for (size_t group_id = 0; group_id < groups.size(); ++group_id) {
        const ValueGroup &group = groups[group_id];
        for (int before : group) {
            move_fact(var, before, next_free_pos);
        }
        assert(in_bounds(next_free_pos, fact_names[var]));
        fact_names[var][next_free_pos] = move(combined_fact_names[group_id]);
        ++next_free_pos;
    }
    int new_domain_size = num_single_values + static_cast<int>(groups.size());
    assert(next_free_pos = new_domain_size);

    // Update domain size.
    fact_names[var].resize(new_domain_size);
    domain_size[var] = new_domain_size;
}

shared_ptr<AbstractTask> DomainAbstractedTaskBuilder::get_task() const {
    return task;
}

shared_ptr<AbstractTask> build_domain_abstracted_task(
    const shared_ptr<AbstractTask> parent,
    const VarToGroups &value_groups) {
    return DomainAbstractedTaskBuilder(parent, value_groups).get_task();
}
}
