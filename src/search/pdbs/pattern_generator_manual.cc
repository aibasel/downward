#include "pattern_generator_manual.h"

#include "pattern_information.h"

#include "../task_proxy.h"

#include "../plugins/plugin.h"
#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace pdbs {
PatternGeneratorManual::PatternGeneratorManual(const plugins::Options &opts)
    : PatternGenerator(opts), pattern(opts.get_list<int>("pattern")) {
}

string PatternGeneratorManual::name() const {
    return "manual pattern generator";
}

PatternInformation PatternGeneratorManual::compute_pattern(
    const shared_ptr<AbstractTask> &task) {
    PatternInformation pattern_info(TaskProxy(*task), move(pattern), log);
    if (log.is_at_least_normal()) {
        log << "Manual pattern: " << pattern_info.get_pattern() << endl;
    }
    return pattern_info;
}

class PatternGeneratorManualFeature : public plugins::TypedFeature<PatternGenerator, PatternGeneratorManual> {
public:
    PatternGeneratorManualFeature() : TypedFeature("manual_pattern") {
        add_list_option<int>(
            "pattern",
            "list of variable numbers of the planning task that should be used as "
            "pattern.");
        add_generator_options_to_feature(*this);
    }
};

static plugins::FeaturePlugin<PatternGeneratorManualFeature> _plugin;
}
