#include "command_line.h"

#include "plan_manager.h"
#include "search_algorithm.h"

#include "parser/lexical_analyzer.h"
#include "parser/syntax_analyzer.h"
#include "plugins/any.h"
#include "plugins/doc_printer.h"
#include "plugins/plugin.h"
#include "utils/logging.h"
#include "utils/strings.h"

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <vector>

using namespace std;

NO_RETURN
static void input_error(const string &msg) {
    cerr << msg << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}


static int parse_int_arg(const string &name, const string &value) {
    try {
        return stoi(value);
    } catch (invalid_argument &) {
        input_error("argument for " + name + " must be an integer");
    } catch (out_of_range &) {
        input_error("argument for " + name + " is out of range");
    }
}

static vector<string> replace_old_style_predefinitions(const vector<string> &args) {
    vector<string> new_args;
    int num_predefinitions = 0;
    bool has_search_argument = false;
    ostringstream new_search_argument;

    for (size_t i = 0; i < args.size(); ++i) {
        string arg = args[i];
        bool is_last = (i == args.size() - 1);

        if (arg == "--evaluator" || arg == "--heuristic" || arg == "--landmarks") {
            if (has_search_argument)
                input_error("predefinitions are forbidden after the '--search' argument");
            if (is_last)
                input_error("missing argument after " + arg);
            ++i;
            vector<string> predefinition = utils::split(args[i], "=", 1);
            if (predefinition.size() < 2)
                input_error("predefinition expects format 'key=definition'");
            string key = predefinition[0];
            string definition = predefinition[1];
            if (!utils::is_alpha_numeric(key))
                input_error("predefinition key has to be alphanumeric: '" + key + "'");
            new_search_argument << "let(" << key << "," << definition << ",";
            num_predefinitions++;
        } else if (arg == "--search") {
            if (has_search_argument)
                input_error("at most one '--search' argument may be specified");
            if (is_last)
                input_error("missing argument after --search");
            ++i;
            arg = args[i];
            new_args.push_back("--search");
            new_search_argument << arg << string(num_predefinitions, ')');
            new_args.push_back(new_search_argument.str());
            has_search_argument = true;
        } else {
            new_args.push_back(arg);
        }
    }
    if (!has_search_argument && num_predefinitions > 0)
        input_error("predefinitions require a '--search' argument");

    return new_args;
}

static shared_ptr<SearchAlgorithm> parse_cmd_line_aux(const vector<string> &args) {
    string plan_filename = "sas_plan";
    int num_previously_generated_plans = 0;
    bool is_part_of_anytime_portfolio = false;

    using SearchPtr = shared_ptr<SearchAlgorithm>;
    SearchPtr search_algorithm = nullptr;
    // TODO: Remove code duplication.
    for (size_t i = 0; i < args.size(); ++i) {
        const string &arg = args[i];
        bool is_last = (i == args.size() - 1);
        if (arg == "--search") {
            if (search_algorithm)
                input_error("multiple --search arguments defined");
            if (is_last)
                input_error("missing argument after --search");
            ++i;
            const string &search_arg = args[i];
            try {
                parser::TokenStream tokens = parser::split_tokens(search_arg);
                parser::ASTNodePtr parsed = parser::parse(tokens);
                parser::DecoratedASTNodePtr decorated = parsed->decorate();
                plugins::Any constructed = decorated->construct();
                search_algorithm = plugins::any_cast<SearchPtr>(constructed);
            } catch (const utils::ContextError &e) {
                input_error(e.get_message());
            }
        } else if (arg == "--help") {
            cout << "Help:" << endl;
            bool txt2tags = false;
            vector<string> plugin_names;
            for (size_t j = i + 1; j < args.size(); ++j) {
                const string &help_arg = args[j];
                if (help_arg == "--txt2tags") {
                    txt2tags = true;
                } else {
                    plugin_names.push_back(help_arg);
                }
            }
            plugins::Registry registry = plugins::RawRegistry::instance()->construct_registry();
            unique_ptr<plugins::DocPrinter> doc_printer;
            if (txt2tags)
                doc_printer = utils::make_unique_ptr<plugins::Txt2TagsPrinter>(
                    cout, registry);
            else
                doc_printer = utils::make_unique_ptr<plugins::PlainPrinter>(
                    cout, registry);
            if (plugin_names.empty()) {
                doc_printer->print_all();
            } else {
                for (const string &name : plugin_names) {
                    doc_printer->print_feature(name);
                }
            }
            cout << "Help output finished." << endl;
            exit(0);
        } else if (arg == "--internal-plan-file") {
            if (is_last)
                input_error("missing argument after --internal-plan-file");
            ++i;
            plan_filename = args[i];
        } else if (arg == "--internal-previous-portfolio-plans") {
            if (is_last)
                input_error("missing argument after --internal-previous-portfolio-plans");
            ++i;
            is_part_of_anytime_portfolio = true;
            num_previously_generated_plans = parse_int_arg(arg, args[i]);
            if (num_previously_generated_plans < 0)
                input_error("argument for --internal-previous-portfolio-plans must be positive");
        } else {
            input_error("unknown option " + arg);
        }
    }

    if (search_algorithm) {
        PlanManager &plan_manager = search_algorithm->get_plan_manager();
        plan_manager.set_plan_filename(plan_filename);
        plan_manager.set_num_previously_generated_plans(num_previously_generated_plans);
        plan_manager.set_is_part_of_anytime_portfolio(is_part_of_anytime_portfolio);
    }
    return search_algorithm;
}

shared_ptr<SearchAlgorithm> parse_cmd_line(
    int argc, const char **argv, bool is_unit_cost) {
    vector<string> args;
    bool active = true;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];

        if (arg == "--if-unit-cost") {
            active = is_unit_cost;
        } else if (arg == "--if-non-unit-cost") {
            active = !is_unit_cost;
        } else if (arg == "--always") {
            active = true;
        } else if (active) {
            args.push_back(arg);
        }
    }
    args = replace_old_style_predefinitions(args);
    return parse_cmd_line_aux(args);
}

static vector<string> complete_args(
    const vector<string> &parsed_args, const string &current_word,
    int /*cursor_pos*/) {
    assert(!parsed_args.empty()); // args[0] is always the program name.
    const string &last_arg = parsed_args.back();
    vector<string> suggestions;
    if (find(parsed_args.begin(), parsed_args.end(), "--help") != parsed_args.end()) {
        suggestions.push_back("--txt2tags");
        plugins::Registry registry = plugins::RawRegistry::instance()->construct_registry();
        for (const shared_ptr<const plugins::Feature> &feature : registry.get_features()) {
            suggestions.push_back(feature->get_key());
        }
    } else if (last_arg == "--internal-plan-file") {
        // Suggest filename starting with current_word

        // Split into directory and file_prefix
        string directory = ".";
        string file_prefix = current_word;
        auto last_slash_pos = current_word.find_last_of("/\\");
        if (last_slash_pos != string::npos) {
            directory = current_word.substr(0, last_slash_pos);
            file_prefix = current_word.substr(last_slash_pos + 1);
        }

        // Add file and directory names to suggestions
        for (const auto &entry : filesystem::directory_iterator(directory)) {
            string path = entry.path().string();

            // Append preferred separator ("/" or "\") to directories base on
            // operating system
            if (entry.is_directory()) {
                path += filesystem::path::preferred_separator;
            }

            // Remove "./" prefix when not present in prefix
            if (last_slash_pos == string::npos && directory == "." && path.starts_with("/\\")) {
                path = path.substr(2);
            }
            suggestions.push_back(path);
        }
    } else if (last_arg == "--internal-previous-portfolio-plans") {
        // no suggestions, integer expected
    } else if (last_arg == "--search") {
        // suggestions in search string based on current_word
        plugins::Registry registry = plugins::RawRegistry::instance()->construct_registry();
        for (const shared_ptr<const plugins::Feature> &feature : registry.get_features()) {
            suggestions.push_back(feature->get_key() + "(");
        }
    } else {
        // not completing an argument
        suggestions.push_back("--help");
        suggestions.push_back("--search");
        suggestions.push_back("--internal-plan-file");
        suggestions.push_back("--internal-previous-portfolio-plans");
        suggestions.push_back("--if-unit-cost");
        suggestions.push_back("--if-non-unit-cost");
        suggestions.push_back("--always");
        // remove suggestions not starting with current_word
    }

    if (!current_word.empty()) {
        // Suggest only words that match with current_word
        suggestions.erase(
            remove_if(suggestions.begin(), suggestions.end(),
                      [&](const string &value) {
                          return !value.starts_with(current_word);
                      }), suggestions.end());
    }
    return suggestions;
}

void handle_tab_completion(int argc, const char **argv) {
    if (argc < 2 || static_cast<string>(argv[1]) != "--bash-complete") {
        return;
    }
    if (argc < 5) {
        input_error(
            "The option --bash-complete is only meant to be called "
            "internally to generate suggestions for tab completion.\n"
            "Usage:\n    ./downward --bash-complete "
            "$COMP_POINT \"$COMP_LINE\" $COMP_CWORD ${COMP_WORDS[@]}\n"
            "where the environment variables have their usual meaning for bash completion:\n"
            "$COMP_POINT is the position of the cursor in the command line.\n"
            "$COMP_LINE is the current command line.\n"
            "$COMP_CWORD is an index into ${COMP_WORDS} of the word under the cursor.\n"
            "$COMP_WORDS is the current command line split into words.\n"
            );
    }
    int cursor_pos = parse_int_arg("COMP_POINT", static_cast<string>(argv[2]));
    const string command_line = static_cast<string>(argv[3]);
    int cursor_word_index = parse_int_arg("COMP_CWORD", static_cast<string>(argv[4]));
    vector<string> words;
    words.reserve(argc - 5);
    for (int i = 5; i < argc; ++i) {
        words.emplace_back(argv[i]);
    }

    vector<string> parsed_args;
    string current_word;
    int pos = 0;
    int end = static_cast<int>(command_line.size());
    int num_words = static_cast<int>(words.size());
    for (int i = 0; i < num_words; ++i) {
        current_word = words[i];
        int word_len = static_cast<int>(current_word.size());
        if (command_line.substr(pos, word_len) != current_word) {
            input_error("Expected '" + current_word + "' in $COMP_LINE at position "
                        + to_string(pos));
        }
        if (i == cursor_word_index) {
            if (cursor_pos < 0 || cursor_pos > word_len) {
                input_error("Inconsistent information in COMP_LINE and COMP_WORDS");
            }
            break;
        }

        // Skip word
        pos += word_len;
        cursor_pos -= word_len;
        parsed_args.push_back(current_word);

        // Skip whitespace between words.
        while (pos < end && isspace(command_line[pos])) {
            ++pos;
            --cursor_pos;
        }
    }

    for (const string &suggestion : complete_args(parsed_args, current_word, cursor_pos)) {
        cout << suggestion << endl;
    }
    // Do not use exit_with here because it would generate additional output.
    exit(0);
}

string usage(const string &progname) {
    return "usage: \n" +
           progname + " [OPTIONS] --search SEARCH < OUTPUT\n\n"
           "* SEARCH (SearchAlgorithm): configuration of the search algorithm\n"
           "* OUTPUT (filename): translator output\n\n"
           "Options:\n"
           "--help [NAME]\n"
           "    Prints help for all heuristics, open lists, etc. called NAME.\n"
           "    Without parameter: prints help for everything available\n"
           "--internal-plan-file FILENAME\n"
           "    Plan will be output to a file called FILENAME\n\n"
           "--internal-previous-portfolio-plans COUNTER\n"
           "    This planner call is part of a portfolio which already created\n"
           "    plan files FILENAME.1 up to FILENAME.COUNTER.\n"
           "    Start enumerating plan files with COUNTER+1, i.e. FILENAME.COUNTER+1\n\n"
           "See https://www.fast-downward.org for details.";
}
