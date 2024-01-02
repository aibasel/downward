#include "null_pruning_method.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

using namespace std;

namespace null_pruning_method {
NullPruningMethod::NullPruningMethod(const utils::Verbosity verbosity)
    : PruningMethod(verbosity) {
}

void NullPruningMethod::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    log << "pruning method: none" << endl;
}

TaskIndependentNullPruningMethod::TaskIndependentNullPruningMethod(const std::string &name, utils::Verbosity verbosity)
                                                                   : TaskIndependentPruningMethod(name, verbosity){}

    using ConcreteProduct = NullPruningMethod;
    using AbstractProduct = PruningMethod;
    using Concrete = TaskIndependentNullPruningMethod;
// TODO issue559 use templates as 'get_task_specific' is EXACTLY the same for all TI_Components
    shared_ptr<AbstractProduct> Concrete::get_task_specific(
            const std::shared_ptr<AbstractTask> &task,
            std::unique_ptr<ComponentMap> &component_map,
            int depth) const {
        shared_ptr<ConcreteProduct> task_specific_x;

        if (component_map->count(static_cast<const TaskIndependentComponent *>(this))) {
            log << std::string(depth, ' ') << "Reusing task specific " << get_product_name() << " '" << name << "'..." << endl;
            task_specific_x = dynamic_pointer_cast<ConcreteProduct>(
                    component_map->at(static_cast<const TaskIndependentComponent *>(this)));
        } else {
            log << std::string(depth, ' ') << "Creating task specific " << get_product_name() << " '" << name << "'..." << endl;
            task_specific_x = create_ts(task, component_map, depth);
            component_map->insert(make_pair<const TaskIndependentComponent *, std::shared_ptr<Component>>
                                          (static_cast<const TaskIndependentComponent *>(this), task_specific_x));
        }
        return task_specific_x;
    }

    std::shared_ptr<ConcreteProduct> Concrete::create_ts(
            [[maybe_unused]] const shared_ptr <AbstractTask> &task,
            [[maybe_unused]] unique_ptr <ComponentMap> &component_map,
            [[maybe_unused]] int depth) const {
        return make_shared<NullPruningMethod>(verbosity);
    }

class TaskIndependentNullPruningMethodFeature : public plugins::TypedFeature<TaskIndependentPruningMethod, TaskIndependentNullPruningMethod> {
public:
    TaskIndependentNullPruningMethodFeature() : TypedFeature("null") {
        // document_group("");
        document_title("No pruning");
        document_synopsis(
            "This is a skeleton method that does not perform any pruning, i.e., "
            "all applicable operators are applied in all expanded states. ");

        add_pruning_options_to_feature(*this, "null_pruning");
    }

    virtual shared_ptr<TaskIndependentNullPruningMethod> create_component(
            const plugins::Options &opts, const utils::Context &) const override {
        return make_shared<TaskIndependentNullPruningMethod>(opts.get<string>("name"),
                                                      opts.get<utils::Verbosity>("verbosity"));
    }
};

static plugins::FeaturePlugin<TaskIndependentNullPruningMethodFeature> _plugin;
}
