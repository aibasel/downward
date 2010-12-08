#include "option_parser.h"
#include <string>
#include <iostream>

using namespace std;

ParseTree::ParseTree():
    parent_(this)
{
}
    
ParseTree::ParseTree(ParseTree* parent, string v, string k):
    value(v),
    key(k),
    parent_(parent)
{
}


ParseTree* ParseTree::get_parent() const {
    return parent_;
}

vector<ParseTree>* ParseTree::get_children() {
    return &children_;
}


void ParseTree::add_child(string value, string key) {
    children_.push_back(ParseTree(this, value, key));
}

ParseTree* ParseTree::last_child() {
    return &children_.back();
}

ParseTree* ParseTree::find_child(std::string key) {
    for (size_t i(0); i != children_.size(); ++i) {
        if (children_[i].key.compare(key) == 0) {
            return &children_[i];
        }
    }
    return 0;
}


bool ParseTree::is_root() const {
    return (this == parent_);
} 

bool ParseTree::operator == (ParseTree& pt){
    vector<ParseTree>* pt_children = pt.get_children();
    vector<ParseTree>* this_children = get_children();
    if (pt.value != value 
        || pt.key != key 
        || pt_children->size() != this_children->size()){
        return false;
    } else {
        for (size_t i(0); i != pt_children->size(); ++i){
            if(pt_children->at(i) != this_children->at(i)){
                return false;
            }
        }
        return true;
    }
}



/*
void OptionParser::parse_cmd_line(char **argv, bool dry_run) {
    for (int i = 1; i < argc; ++i) {
        string arg = string(argv[i]);
        if (arg.compare("--heuristic") == 0) {
            ++i;
            OptionParser::instance()->predefine_heuristic(argv[i]);
        } else if (arg.compare("--landmarks") == 0) {
            ++i;
            OptionParser::instance()->predefine_lm_graph(argv[i]);
        } else if (arg.compare("--search") == 0) {
            ++i;
            engine = OptionParser::instance()->parse_search_engine(argv[i]);
        } else if (arg.compare("--random-seed") == 0) {
            ++i;
            srand(atoi(argv[i]));
            cout << "random seed " << argv[i] << endl;
        } else {
            cerr << "unknown option " << arg << endl << endl;
            cout << usage << endl;
            exit(1);
        }
    }
}
*/

bool ParseTree::operator != (ParseTree& pt) {
    return !(this->operator== (pt));
}


OptionParser::OptionParser(const string config):
    parse_tree(generate_parse_tree(config)),
    next_unparsed_argument(parse_tree.get_children()->begin())
{
}



OptionParser::OptionParser(ParseTree pt):
    parse_tree(pt),
    next_unparsed_argument(parse_tree.get_children()->begin())
{
}

string OptionParser::to_lowercase(const string& s){
    string t;
    for (string::const_iterator i = s.begin(); i != s.end(); ++i)
        t += tolower(*i);
    return t;
}

ParseTree OptionParser::generate_parse_tree(const string config) {
    ParseTree root;
    ParseTree* cur_node(&root);
    string buffer(""), key("");
    for (size_t i(0); i != config.size(); ++i){
        char next = config.at(i);
        if((next == '(' || next == ')' || next == ',') && buffer.size() > 0){
            cur_node->add_child(buffer, key);
            buffer.clear();
            key.clear();
        }
        switch (next){
        case ' ':
            break;
        case '(':
            cur_node = cur_node->last_child();
            break;
        case ')':
            cur_node = cur_node->get_parent();
            break;
        case '[':
            cur_node->add_child("list", key);
            key.clear();
            cur_node = cur_node->last_child();
            break;
        case ']':
            if(buffer.size() > 0) {
                cur_node->add_child(buffer, key);
                buffer.clear();
                key.clear();
            }
            cur_node = cur_node->get_parent();
            break;
        case ',':
            break;
        case '=':
            key = buffer;
            buffer.clear();
            break;
        default:
            buffer.push_back(next);
            break;
        }    
    }
        
        
    return *root.last_child();
}

