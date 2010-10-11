#ifndef OPEN_LIST_PARSER_H
#define OPEN_LIST_PARSER_H

template <class Entry>
class OpenListParser {
private:
    typedef const std::vector<std::string> &ConfigRef;
    typedef OpenList<Entry> * (*OpenListFactory)(ConfigRef, int, int &, bool);

    std::map<std::string, OpenListFactory> open_list_map;

    static OpenListParser *instance_;
    OpenListParser() {}
    OpenListParser(const OpenListParser &);
    void register_open_list(
        const std::string &key,
        OpenList<Entry> *func(const vector<string> &, int, int &, bool));

public:
    static OpenListParser *instance();
    OpenList<Entry> *parse_open_list(const std::vector<std::string> &input,
                                     int start, int &end, bool dry_run);
    bool knows_open_list(const std::string &name);
};

#include "open_list_parser.cc"

#endif
