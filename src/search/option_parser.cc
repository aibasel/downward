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

vector<ParseTree> const *ParseTree::get_children() const {
    return &children_;
}


void ParseTree::add_child(string value, string key) {
    children_.push_back(ParseTree(this, value, key));
}

ParseTree* ParseTree::last_child() {
    return &children_.back();
}

ParseTree* ParseTree::find_child(string key) {
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

std::ostream& operator<< (std::ostream& o, const ParseTree &pt)
 {
     return o << "implement << for ParseTree!" << pt.value;
 }

bool ParseTree::operator == (const ParseTree &pt){
    vector<ParseTree> const* pt_children = pt.get_children();
    vector<ParseTree> const* this_children = get_children();
    if (pt.value != value 
        || pt.key != key 
        || pt_children->size() != this_children->size()){
        return false;
    } else {
        for (size_t i(0); i != pt_children->size(); ++i){
            ParseTree c1 = pt_children->at(i);
            ParseTree c2 = this_children->at(i);
            if(c1 != c2){
                return false;
            }
        }
        return true;
    }
}

bool ParseTree::operator != (const ParseTree &pt) {
    return !(this->operator== (pt));
}



ParseError::ParseError(string _msg, ParseTree pt)
    : msg(_msg),
      parse_tree(pt)
{
}



HelpElement::HelpElement(string k, string h, string t_n) 
    : kwd(k),
      help(h),
      type_name(t_n)
{
}

void OptionParser::error(string msg) {
    throw ParseError(msg);
}

void OptionParser::warning(string msg) {
    cout << "Parser Warning: " << msg << endl;
}


/*
Functions for printing help
*/

void OptionParser::set_help_mode(bool m) {
    dry_run_ = dry_run_ && m;
    help_mode_ = m;
}

template <class T>
static void get_help_templ(const ParseTree &pt) {
    if (Registry<T>::instance()->contains(pt.value)) {
        cout << pt.value << " is a " << TypeNamer<T>::name() 
             << endl << "Usage: " << endl;        
        OptionParser p(pt, true);
        p.set_help_mode(true);
        p.start_parsing<T>();
        cout << endl;
    }
}

static void get_help(string k) {
    ParseTree pt;
    pt.value = k;
    get_help_templ<SearchEngine *>(pt);
    get_help_templ<Heuristic *>(pt);
    get_help_templ<ScalarEvaluator *>(pt);
    get_help_templ<Synergy *>(pt);
    get_help_templ<LandmarksGraph *>(pt);
    cout << "For help with open lists, please use --help with no specifier."
         << "Open lists are only registered when needed by other objects."
         << endl;
}

template <class T>
static void get_full_help_templ() {
    cout << endl << "Help for " << TypeNamer<T>::name() << "s:" << endl << endl;
    vector<string> keys = Registry<T>::instance()->get_keys();
    for(size_t i(0); i != keys.size(); ++i) {
        ParseTree pt;
        pt.value = keys[i];
        get_help_templ<T>(pt);
    }
}

static void get_full_help() {
    get_full_help_templ<SearchEngine *>();
    get_full_help_templ<Heuristic *>();
    get_full_help_templ<ScalarEvaluator *>();
    get_full_help_templ<Synergy *>();
    get_full_help_templ<LandmarksGraph *>();
    get_full_help_templ<OpenList<short *> *>();
}

SearchEngine *OptionParser::parse_cmd_line(
    int argc, const char **argv, bool dry_run) {
    SearchEngine *engine(0);
    for (int i = 1; i < argc; ++i) {
        string arg = string(argv[i]);
        if (arg.compare("--heuristic") == 0) {
            ++i;
            OptionParser::predefine<Heuristic *>(argv[i]);
        } else if (arg.compare("--landmarks") == 0) {
            ++i;
            OptionParser::predefine<LandmarksGraph *>(argv[i]);
        } else if (arg.compare("--search") == 0) {
            ++i;
            OptionParser p(argv[i], dry_run);
            engine = p.start_parsing<SearchEngine *>();
        } else if (arg.compare("--random-seed") == 0) {
            ++i;
            srand(atoi(argv[i]));
            cout << "random seed " << argv[i] << endl;
        } else if ((arg.compare("--help") == 0) && dry_run) {
            cout << "Help:" << endl;
            if( i+1 < argc) {
                string helpiand = string(argv[i+1]);
                get_help(helpiand);
            } else {
                get_full_help();
            }
            cout << "Help finished." << endl;
            exit(0);
        } else {
            cerr << "unknown option " << arg << endl << endl;
            string usage =
                "usage: \n" +
                string(argv[0]) + " [OPTIONS] --search SEARCH < OUTPUT\n\n"
                "* SEARCH (SearchEngine): configuration of the search algorithm\n"
                "* OUTPUT (filename): preprocessor output\n\n"
                "Options:\n"
                "--heuristic HEURISTIC_PREDEFINITION\n"
                "    Predefines a heuristic that can afterwards be referenced\n"
                "    by the name that is specified in the definition.\n"
                "--random-seed SEED\n"
                "    Use random seed SEED\n\n"
                "See http://www.fast-downward.org/ for details.";
            cout << usage << endl;
            exit(1);
        }
    }
    return engine;
}



OptionParser::OptionParser(const string config, bool dr):
    parse_tree(generate_parse_tree(config)),
    dry_run_(dr),
    help_mode_(false),
    next_unparsed_argument(parse_tree.get_children()->begin())
{
}


OptionParser::OptionParser(ParseTree pt, bool dr):
    parse_tree(pt),
    dry_run_(dr),
    help_mode_(false),
    next_unparsed_argument(parse_tree.get_children()->begin())
{
}

                           

void OptionParser::add_enum_option(string k, 
                                   const vector<string >& enumeration, 
                                   string def_val, string h) {
    if(help_mode_) {
        string enum_descr = "{";
        for (size_t i(0); i != enumeration.size(); ++i) {
            enum_descr += enumeration[i];
            if( i != enumeration.size() - 1) {
                enum_descr += ", ";
            }
        }
        enum_descr += "}";
        
        helpers.push_back(HelpElement(k, h, enum_descr));
        if(def_val.compare("") != 0){
            helpers.back().default_value = def_val;
        }
        return;
    }
    
    
    //first parse the corresponding string like a normal argument... 
    if (def_val.compare("") != 0) {
        add_option<string>(k, def_val, h);
    } else {
        add_option<string>(k, h);
    }

    //...then map that string to its position in the enumeration vector
    string name = opts.get<string>(k);
    vector<string>::const_iterator it = 
        find(enumeration.begin(), enumeration.end(), name);
    if (it == enumeration.end()) {
        error("invalid enum argument");
    }
    opts.set(k, it - enumeration.begin());            
}

Options OptionParser::parse() {
    if(help_mode_) {
        cout << parse_tree.value << "(";
        for (size_t i(0); i != helpers.size(); ++i) {
            cout << helpers[i].kwd 
                 << (helpers[i].default_value.compare("") != 0 ? " = " : "") 
                 << helpers[i].default_value;
            if( i != helpers.size() - 1) {
                cout << ", ";
            }
        }
        cout << ")" << endl;
        for (size_t i(0); i != helpers.size(); ++i) {
            cout << helpers[i].kwd << "(" << helpers[i].type_name << "): "
                 << helpers[i].help << endl;
        }
    }
    //first check if there were any arguments with invalid keywords
    vector<ParseTree>* pt_children = parse_tree.get_children();
    for (size_t i(0); i != pt_children->size(); ++i) {
        if (pt_children->at(i).key.compare("") != 0 &&
            find(valid_keys.begin(), 
                 valid_keys.end(), 
                 pt_children->at(i).key) == valid_keys.end()) {
            error("invalid keyword");
        }
    }    
    return opts;
}

bool OptionParser::dry_run() {
    return dry_run_;
}

bool OptionParser::help_mode() {
    return help_mode_;
}

void OptionParser::set_parse_tree(const ParseTree& pt) {
    parse_tree = pt;
}

ParseTree* OptionParser::get_parse_tree() {
    return &parse_tree;
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
            if(cur_node->is_root()) 
                throw ParseError("missing (", *cur_node);
            cur_node = cur_node->get_parent();
            break;
        case '[':
            if(!buffer.empty())
                throw ParseError("misplaced opening bracket", *cur_node);
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
            if(cur_node->value.compare("list") != 0)
                throw ParseError("mismatched brackets", *cur_node);
            break;
        case ',':
            break;
        case '=':
            if (key.empty())
                throw ParseError("expected keyword before =", *cur_node);
            key = buffer;
            buffer.clear();
            break;
        default:
            buffer.push_back(tolower(next));
            break;
        }    
    }
    if (!cur_node->is_root())
        throw ParseError("missing )", *cur_node);
        
    return *root.last_child();
}



