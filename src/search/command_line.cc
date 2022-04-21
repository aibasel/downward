#include "command_line.h"

#include "option_parser.h"
#include "plan_manager.h"
#include "search_engine.h"

#include "options/doc_printer.h"
#include "options/predefinitions.h"
#include "options/registries.h"
#include "utils/strings.h"

#include <algorithm>
#include <vector>

using namespace std;

ArgError::ArgError(const string &msg)
    : msg(msg) {
}

void ArgError::print() const {
    cerr << "argument error: " << msg << endl;
}


static string sanitize_arg_string(string s) {
    // Convert newlines to spaces.
    replace(s.begin(), s.end(), '\n', ' ');
    // Convert string to lower case.
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static int parse_int_arg(const string &name, const string &value) {
    try {
        return stoi(value);
    } catch (invalid_argument &) {
        throw ArgError("argument for " + name + " must be an integer");
    } catch (out_of_range &) {
        throw ArgError("argument for " + name + " is out of range");
    }
}

static shared_ptr<SearchEngine> parse_cmd_line_aux(
    const vector<string> &args, options::Registry &registry, bool dry_run) {
    string plan_filename = "sas_plan";
    int num_previously_generated_plans = 0;
    bool is_part_of_anytime_portfolio = false;
    options::Predefinitions predefinitions;

    shared_ptr<SearchEngine> engine;
    /*
      Note that we don’t sanitize all arguments beforehand because filenames should remain as-is
      (no conversion to lower-case, no conversion of newlines to spaces).
    */
    // TODO: Remove code duplication.
    for (size_t i = 0; i < args.size(); ++i) {
        string arg = sanitize_arg_string(args[i]);
        bool is_last = (i == args.size() - 1);
        if (arg == "--search") {
            if (is_last)
                throw ArgError("missing argument after --search");
            ++i;
            OptionParser parser(sanitize_arg_string(args[i]), registry,
                                predefinitions, dry_run);
            engine = parser.start_parsing<shared_ptr<SearchEngine>>();
        } else if (arg == "--help" && dry_run) {
            cout << "Help:" << endl;
            bool txt2tags = false;
            vector<string> plugin_names;
            for (size_t j = i + 1; j < args.size(); ++j) {
                string help_arg = sanitize_arg_string(args[j]);
                if (help_arg == "--txt2tags") {
                    txt2tags = true;
                } else {
                    plugin_names.push_back(help_arg);
                }
            }
            unique_ptr<options::DocPrinter> doc_printer;
            if (txt2tags)
                doc_printer = utils::make_unique_ptr<options::Txt2TagsPrinter>(
                    cout, registry);
            else
                doc_printer = utils::make_unique_ptr<options::PlainPrinter>(
                    cout, registry);
            if (plugin_names.empty()) {
                doc_printer->print_all();
            } else {
                for (const string &name : plugin_names) {
                    doc_printer->print_plugin(name);
                }
            }
            cout << "Help output finished." << endl;
            exit(0);
        } else if (arg == "--internal-plan-file") {
            if (is_last)
                throw ArgError("missing argument after --internal-plan-file");
            ++i;
            plan_filename = args[i];
        } else if (arg == "--internal-previous-portfolio-plans") {
            if (is_last)
                throw ArgError("missing argument after --internal-previous-portfolio-plans");
            ++i;
            is_part_of_anytime_portfolio = true;
            num_previously_generated_plans = parse_int_arg(arg, args[i]);
            if (num_previously_generated_plans < 0)
                throw ArgError("argument for --internal-previous-portfolio-plans must be positive");
        } else if (utils::startswith(arg, "--") &&
                   registry.is_predefinition(arg.substr(2))) {
            if (is_last)
                throw ArgError("missing argument after " + arg);
            ++i;
            registry.handle_predefinition(arg.substr(2),
                                          sanitize_arg_string(args[i]),
                                          predefinitions, dry_run);
        } else {
            throw ArgError("unknown option " + arg);
        }
    }

    if (engine) {
        PlanManager &plan_manager = engine->get_plan_manager();
        plan_manager.set_plan_filename(plan_filename);
        plan_manager.set_num_previously_generated_plans(num_previously_generated_plans);
        plan_manager.set_is_part_of_anytime_portfolio(is_part_of_anytime_portfolio);
    }
    return engine;
}


shared_ptr<SearchEngine> parse_cmd_line(
    int argc, const char **argv, options::Registry &registry, bool dry_run, bool is_unit_cost) {
    vector<string> args;
    bool active = true;
    for (int i = 1; i < argc; ++i) {
        string arg = sanitize_arg_string(argv[i]);

        if (arg == "--if-unit-cost") {
            active = is_unit_cost;
        } else if (arg == "--if-non-unit-cost") {
            active = !is_unit_cost;
        } else if (arg == "--always") {
            active = true;
        } else if (active) {
            // We use the unsanitized arguments because sanitizing is inappropriate for things like filenames.
            args.push_back(argv[i]);
        }
    }
    return parse_cmd_line_aux(args, registry, dry_run);
}


string usage(const string &progname) {
    return "usage: \n" +
           progname + " [OPTIONS] --search SEARCH < OUTPUT\n\n"
           "* SEARCH (SearchEngine): configuration of the search algorithm\n"
           "* OUTPUT (filename): translator output\n\n"
           "Options:\n"
           "--help [NAME]\n"
           "    Prints help for all heuristics, open lists, etc. called NAME.\n"
           "    Without parameter: prints help for everything available\n"
           "--landmarks LANDMARKS_PREDEFINITION\n"
           "    Predefines a set of landmarks that can afterwards be referenced\n"
           "    by the name that is specified in the definition.\n"
           "--evaluator EVALUATOR_PREDEFINITION\n"
           "    Predefines an evaluator that can afterwards be referenced\n"
           "    by the name that is specified in the definition.\n"
           "--internal-plan-file FILENAME\n"
           "    Plan will be output to a file called FILENAME\n\n"
           "--internal-previous-portfolio-plans COUNTER\n"
           "    This planner call is part of a portfolio which already created\n"
           "    plan files FILENAME.1 up to FILENAME.COUNTER.\n"
           "    Start enumerating plan files with COUNTER+1, i.e. FILENAME.COUNTER+1\n\n"
           "See https://www.fast-downward.org for details.";
}
