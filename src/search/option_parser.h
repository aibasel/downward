#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#include <vector>
#include <map>
#include <set>
#include <string>

#include "state_var_t.h"

class Heuristic;
class ScalarEvaluator;
class SearchEngine;
template<class Entry>
class OpenList;
class LandmarksGraph;

struct ParseError {
    int pos;
    ParseError(int ii) {pos = ii; }
};

class NamedOptionParser {
    std::map<std::string, bool *> bool_options;
    std::map<std::string, float *> float_options;
    std::map<std::string, int *> int_options;
    std::map<std::string, double *> double_options;
    std::map<std::string, ScalarEvaluator **> scalar_evaluator_options;
    std::map<std::string, std::vector<Heuristic *> *> heuristic_list_options;
    std::map<std::string, std::string *> str_options;
    std::set<std::string> can_be_none;
    std::set<std::string> can_be_infinity;

    std::map<std::string, std::string> help;

    void parse_bool_option(const std::vector<std::string> &config,
                           int start, int &end);
    void parse_double_option(const std::vector<std::string> &config,
                             int start, int &end);
    void parse_int_option(const std::vector<std::string> &config,
                          int start, int &end);
    void parse_scalar_evaluator_option(const std::vector<std::string> &config,
                                       int start, int &end, bool dry_run);
    void parse_heuristic_list_option(const std::vector<std::string> &config,
                                     int start, int &end, bool dry_run);
    void parse_string_option(const std::vector<std::string> &config,
                             int start, int &end);
public:
    void add_bool_option(
        const std::string &name, bool *var, const std::string &desc);
    void add_double_option(
        const std::string &name, double *var, const std::string &desc);
    void add_int_option(
        const std::string &name, int *var, const std::string &desc,
        bool allow_infinity = false);
    void add_scalar_evaluator_option(
        const std::string &name, ScalarEvaluator **var,
        const std::string &desc, bool allow_none = false);
    void add_heuristic_list_option(
        const std::string &name, std::vector<Heuristic *> *var,
        const std::string &desc);
    void add_string_option(
        const std::string &name, std::string *var,
        const std::string &desc);

    void parse_options(const std::vector<std::string> &config,
                       int start, int &end, bool dry_run);
};

class OptionParser {
public:
    typedef const std::vector<std::string> &ConfigRef;
    typedef SearchEngine *(*EngineFactory)(
        ConfigRef, int, int &, bool);
    typedef ScalarEvaluator *(*ScalarEvalFactory)(
        ConfigRef, int, int &, bool);
    typedef void (*SynergyFactory)(
        ConfigRef, int, int &, std::vector<Heuristic *> &);
    typedef OpenList<state_var_t *> *(*OpenListFactory)(
        ConfigRef, int, int &);
    typedef LandmarksGraph *(*LandmarkGraphFactory)(
        ConfigRef, int, int &, bool);
private:
    std::map<std::string, Heuristic *> predefined_heuristics;
    std::map<std::string, LandmarksGraph *> predefined_lm_graphs;

    std::map<std::string, EngineFactory> engine_map;
    std::map<std::string, ScalarEvalFactory> scalar_evaluator_map;
    std::map<std::string, SynergyFactory> synergy_map;
    std::map<std::string, LandmarkGraphFactory> lm_graph_map;

    static OptionParser *instance_;
    OptionParser() {}
    OptionParser(const OptionParser &);

    void tokenize_options(const char *str, std::vector<std::string> &tokens);
    std::string strip_and_to_lower(const char *begin, const char *end);
    void print_parse_error(const std::vector<std::string> &tokens,
                           ParseError &err);

    void predefine_heuristic(const std::vector<std::string> &input);
    void predefine_lm_graph(const std::vector<std::string> &input);

public:
    static OptionParser *instance();

    void register_search_engine(const std::string &key, EngineFactory func);
    void register_scalar_evaluator(
        const std::string &key, ScalarEvalFactory func);
    void register_lm_graph_factory(
        const std::string &key, LandmarkGraphFactory func);
    void register_synergy(const std::string &key, SynergyFactory func);
    void predefine_heuristic(const char *str);
    void predefine_lm_graph(const char *str);

    ScalarEvaluator *parse_scalar_evaluator(
        const std::vector<std::string> &input, int start, int &end,
        bool dry_run);
    LandmarksGraph *parse_lm_graph(
        const std::vector<std::string> &input, int start, int &end,
        bool dry_run);
    Heuristic *parse_heuristic(const std::vector<std::string> &input,
                               int start, int &end, bool dry_run);
    bool knows_scalar_evaluator(const std::string &name) const;
    bool knows_lm_graph(const std::string &name) const;
    bool knows_search_engine(const std::string &name) const;
    SearchEngine *parse_search_engine(const char *str);
    SearchEngine *parse_search_engine(const std::vector<std::string> &input,
                                      int start, int &end, bool dry_run);
    void parse_synergy_heuristics(const std::vector<std::string> &input,
                                  int start, int &end,
                                  std::vector<Heuristic *> &heuristics);

    double parse_double(const std::vector<std::string> &config,
                        int start, int &end);

    int parse_int(const std::vector<std::string> &config,
                  int start, int &end);

    void parse_heuristic_list(const std::vector<std::string> &input,
                              int start, int &end, bool only_one_eval,
                              std::vector<Heuristic *> &heuristics,
                              bool dry_run);
    void parse_landmark_graph_list(const std::vector<std::string> &input,
                                   int start, int &end, bool only_one,
                                   std::vector<LandmarksGraph *> &lm_graphs,
                                   bool dry_run);
    void parse_scalar_evaluator_list(const std::vector<std::string> &input,
                                     int start, int &end, bool only_one_eval,
                                     std::vector<ScalarEvaluator *> &evals,
                                     bool dry_run);
    void set_end_for_simple_config(const std::vector<std::string> &config,
                                   int start, int &end);
    void parse_evals_and_options(
        const std::vector<std::string> &config,
        int start, int &end,
        std::vector<ScalarEvaluator *> &evaluators,
        NamedOptionParser &option_parser,
        bool only_one_eval, bool dry_run);
    void parse_search_engine_list(
        const std::vector<std::string> &input, int start,
        int &end, bool only_one,
        std::vector<int> &engine_config_start,
        bool dry_run);
};

#endif
