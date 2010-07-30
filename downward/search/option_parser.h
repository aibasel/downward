#ifndef OPTION_PARSER_H
#define OPTION_PARSER_H

#include <vector>
#include <map>
#include <string>

typedef short state_var_t;

class Heuristic;
class ScalarEvaluator;
class SearchEngine;
template<class Entry> class OpenList;

struct ParseError {
    int pos;
    ParseError(int ii) { pos = ii; }
};

class NamedOptionParser {
    std::map<std::string, bool *> bool_options;
    std::map<std::string, float *> float_options;
    std::map<std::string, int *> int_options;
    std::map<std::string, ScalarEvaluator *> scalar_evaluator_options;
    std::map<std::string, std::vector<Heuristic *> *> heuristic_list_options;
    std::map<std::string, std::string *> str_options;
     
    std::map<std::string, std::string> help;

    void parse_bool_option(const std::vector<std::string> &config, 
                           int start, int &end);
    void parse_int_option(const std::vector<std::string> &config, 
                           int start, int &end);
    void parse_scalar_evaluator_option(const std::vector<std::string> &config, 
                           int start, int &end);
    void parse_heuristic_list_option(const std::vector<std::string> &config, 
                           int start, int &end);
    void parse_string_option(const std::vector<std::string> &config, 
                           int start, int &end);
    public:
        void add_bool_option(std::string name, bool *var, std::string desc);
        void add_int_option(std::string name, int *var, std::string desc);
        void add_scalar_evaluator_option(std::string name, 
            ScalarEvaluator *var, std::string desc);
        void add_heuristic_list_option(std::string name, 
            std::vector<Heuristic *> *var, std::string desc);
        void add_string_option(std::string name, std::string *var, 
            std::string desc);

        void parse_options(const std::vector<std::string> &config, 
                           int start, int &end);
};

class OptionParser {
    typedef const std::vector<std::string> & ConfigRef;
    typedef SearchEngine* (*EngineCreatorFunc)(ConfigRef, int, int&);
    typedef ScalarEvaluator* (*ScalarEvalCreatorFunc)(ConfigRef, int, int&);
    typedef void (*SynergyCreatorFunc)(ConfigRef, int, int&, 
                                       std::vector<Heuristic *> &);
    typedef 
        OpenList<state_var_t *>* (*OpenListCreatorFunc)(ConfigRef, int, int&);

    std::map<std::string, Heuristic*> predefined_heuristics; 
    std::map<std::string, EngineCreatorFunc> engine_map; 
    std::map<std::string, ScalarEvalCreatorFunc> scalar_evaluator_map; 
    std::map<std::string, SynergyCreatorFunc> synergy_map; 
private:
    static OptionParser* instance_;
    OptionParser() {}
    OptionParser(const OptionParser&);
    
public:
    static OptionParser* instance();

    void register_search_engine(std::string key, 
            SearchEngine* func(const std::vector<std::string> &, int, int&));
    void register_scalar_evaluator(std::string key, 
            ScalarEvaluator* func(const std::vector<std::string> &, int, int&));
    void register_synergy(std::string key, 
            void func(const std::vector<std::string> &, int, int&,
                      std::vector<Heuristic *> &));
    void predefine_heuristic(const std::vector<std::string> &input);

    ScalarEvaluator* parse_scalar_evaluator(const std::vector<std::string> &input, 
        int start, int & end);
    Heuristic* parse_heuristic(const std::vector<std::string> &input, 
        int start, int & end);
    bool knows_scalar_evaluator(std::string name);
    SearchEngine* parse_search_engine(const std::vector<std::string> &input, 
        int start, int & end);
    void parse_synergy_heuristics(const std::vector<std::string> &input, 
                                  int start, int &end, 
                                  std::vector<Heuristic *> &heuristics);

    int parse_int(const std::vector<std::string> &config, 
                  int start, int &end);

    void parse_heuristic_list(const std::vector<std::string> &input, 
                              int start, int & end, bool only_one_eval,
                              std::vector<Heuristic *> &heuristics);
    void parse_scalar_evaluator_list(const std::vector<std::string> &input, 
                                     int start, int & end, bool only_one_eval,
                                     std::vector<ScalarEvaluator *> &evals);
    void set_end_for_simple_config(const std::vector<std::string> &config, 
                                     int start, int &end);
    void parse_evals_and_options(const std::vector<std::string> &config, 
                     int start, int &end,
                     std::vector<ScalarEvaluator *> &evaluators, 
                     NamedOptionParser &option_parser,
                     bool only_one_eval=false);
};


#endif
