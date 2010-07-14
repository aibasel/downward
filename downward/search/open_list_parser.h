#ifndef OPEN_LIST_PARSER_H
#define OPEN_LIST_PARSER_H

template <class Entry>
class OpenListParser {
private:
    typedef const std::vector<std::string> & ConfigRef;
    typedef 
        OpenList<Entry>* (*OpenListCreatorFunc)(ConfigRef, int, int&);

    std::map<std::string, OpenListCreatorFunc> open_list_map; 

    static OpenListParser* instance_;
    OpenListParser() {}
    OpenListParser(const OpenListParser&);
    void register_open_list(std::string key, 
        OpenList<Entry>* func(const vector<string> &, int, int&));

public:
    static OpenListParser* instance();
    OpenList<Entry>* parse_open_list(const std::vector<std::string> &input, 
        int start, int & end);
    bool knows_open_list(std::string name);

};

#include "open_list_parser.cc"

#endif

