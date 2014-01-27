#include <algorithm>
#include <string>
#include <vector>
#include "option_parser_util.h"


using namespace std;

void DocStore::register_object(string k, string type) {
    transform(k.begin(), k.end(), k.begin(), ::tolower); //k to lowercase
    registered[k] = DocStruct();
    registered[k].type = type;
    registered[k].full_name = k;
    registered[k].synopsis = "";
}


void DocStore::add_arg(string k,
                       string arg_name,
                       string help,
                       string type,
                       string default_value,
                       bool mandatory,
                       ValueExplanations value_explanations) {
    registered[k].arg_help.push_back(
        ArgumentInfo(arg_name, help, type, default_value, mandatory,
                     value_explanations));
}

void DocStore::add_value_explanations(string k,
                                      string arg_name,
                                      ValueExplanations value_explanations) {
    vector<ArgumentInfo> &args = registered[k].arg_help;
    for (int i = 0; i != args.size(); ++i) {
        if (args[i].kwd.compare(arg_name) == 0) {
            args[i].value_explanations = value_explanations;
            break;
        }
    }
}


void DocStore::set_synopsis(string k,
                            string name, string description) {
    registered[k].full_name = name;
    registered[k].synopsis = description;
}

void DocStore::add_property(string k,
                            string name, string description) {
    registered[k].property_help.push_back(PropertyInfo(name, description));
}

void DocStore::add_feature(string k,
                           string feature, string description) {
    registered[k].support_help.push_back(LanguageSupportInfo(feature,
                                                             description));
}

void DocStore::add_note(string k,
                        string name, string description, bool long_text) {
    registered[k].notes.push_back(NoteInfo(name, description, long_text));
}

void DocStore::hide(std::string k) {
    registered[k].hidden = true;
}


bool DocStore::contains(string k) {
    return registered.find(k) != registered.end();
}

DocStruct DocStore::get(string k) {
    return registered[k];
}

vector<string> DocStore::get_keys() {
    vector<string> keys;
    for (map<string, DocStruct>::iterator it =
             registered.begin();
         it != registered.end(); ++it) {
        keys.push_back(it->first);
    }
    return keys;
}

vector<string> DocStore::get_types() {
    vector<string> types;
    for (map<string, DocStruct>::iterator it =
             registered.begin();
         it != registered.end(); ++it) {
        if (find(types.begin(), types.end(), it->second.type)
            == types.end()) {
            types.push_back(it->second.type);
        }
    }
    return types;
}


DocPrinter::DocPrinter(ostream &out)
    : os(out) {
}

DocPrinter::~DocPrinter() {
}


void DocPrinter::print_all() {
    DocStore *ds = DocStore::instance();
    vector<string> types = ds->get_types();
    for (size_t n(0); n != types.size(); ++n) {
        // Entries for the category itself have an empty type
        if (!types[n].empty())
            print_category(types[n]);
    }
}

void DocPrinter::print_category(string category_name) {
    print_category_header(category_name);
    DocStore *ds = DocStore::instance();
    print_element("", ds->get(category_name));
    vector<string> keys = ds->get_keys();
    for (size_t i(0); i != keys.size(); ++i) {
        DocStruct info = ds->get(keys[i]);
        if (info.type.compare(category_name) != 0
            || info.hidden)
            continue;
        print_element(keys[i], info);
    }
    print_category_footer();
}

void DocPrinter::print_element(string call_name, const DocStruct &info) {
    print_synopsis(info);
    print_usage(call_name, info);
    print_arguments(info);
    print_notes(info);
    print_language_features(info);
    print_properties(info);
}

Txt2TagsPrinter::Txt2TagsPrinter(ostream &out)
    : DocPrinter(out) {
}

Txt2TagsPrinter::~Txt2TagsPrinter() {
}

void Txt2TagsPrinter::print_synopsis(const DocStruct &info) {
    if (!info.full_name.empty())
        os << "== " << info.full_name << " ==" << endl;
    if (!info.synopsis.empty())
        os << info.synopsis << endl;
}

void Txt2TagsPrinter::print_usage(string call_name, const DocStruct &info) {
    if (!call_name.empty()) {
        os << "``` " << call_name << "(";
        for (size_t j(0); j != info.arg_help.size(); ++j) {
            ArgumentInfo arg = info.arg_help[j];
            os << arg.kwd;
            if (!info.arg_help[j].default_value.empty()) {
                os << "=" << info.arg_help[j].default_value;
            } else if (!info.arg_help[j].mandatory) {
                os << "=None";
            }
            if (j != info.arg_help.size() - 1)
                os << ", ";
        }
        os << ")" << endl << endl << endl;
    }
}

static bool is_call(string s) {
    return s.find("(") != string::npos;
}

void Txt2TagsPrinter::print_arguments(const DocStruct &info) {
    for (size_t j(0); j != info.arg_help.size(); ++j) {
        ArgumentInfo arg = info.arg_help[j];
        os << "- //" << arg.kwd << "// ("
           << arg.type_name << "): "
           << arg.help << endl;
        if (!arg.value_explanations.empty()) {
            for (size_t k(0); k != arg.value_explanations.size(); ++k) {
                pair<string, string> explanation =
                    arg.value_explanations[k];
                if (is_call(explanation.first)) {
                    os << endl << "```" << endl << explanation.first << endl << "```" << endl
                       << " " << explanation.second << endl;
                } else {
                    os << " - ``" << explanation.first << "``: "
                       << explanation.second << endl;
                }
            }
        }
    }
}

void Txt2TagsPrinter::print_notes(const DocStruct &info) {
    for (size_t j(0); j != info.notes.size(); ++j) {
        NoteInfo note = info.notes[j];
        if (note.long_text) {
            os << "=== " << note.name << " ===" << endl
               << note.description << endl << endl;
        } else {
            os << "**" << note.name << ":** " << note.description << endl << endl;
        }
    }
}

void Txt2TagsPrinter::print_language_features(const DocStruct &info) {
    if (!info.support_help.empty()) {
        os << "Language features supported:" << endl;
    }
    for (size_t j(0); j != info.support_help.size(); ++j) {
        LanguageSupportInfo ls = info.support_help[j];
        os << "- **" << ls.feature << ":** " << ls.description << endl;
    }
}

void Txt2TagsPrinter::print_properties(const DocStruct &info) {
    if (!info.property_help.empty()) {
        os << "Properties:" << endl;
    }
    for (size_t j(0); j != info.property_help.size(); ++j) {
        PropertyInfo p = info.property_help[j];
        os << "- **" << p.property << ":** " << p.description << endl;
    }
}


void Txt2TagsPrinter::print_category_header(string category_name) {
    os << ">>>>CATEGORY: " << category_name << "<<<<" << endl;
}

void Txt2TagsPrinter::print_category_footer() {
    os << endl
       << ">>>>CATEGORYEND<<<<" << endl;
}

PlainPrinter::PlainPrinter(ostream &out, bool pa)
    : DocPrinter(out),
      print_all(pa) {
}

PlainPrinter::~PlainPrinter() {
}

void PlainPrinter::print_synopsis(const DocStruct &info) {
    if (!info.full_name.empty())
        os << "== " << info.full_name << " ==" << endl;
    if (print_all && !info.synopsis.empty()) {
        os << info.synopsis << endl;
    }
}

void PlainPrinter::print_usage(string call_name, const DocStruct &info) {
    if (!call_name.empty()) {
        os << call_name << "(";
        for (size_t j(0); j != info.arg_help.size(); ++j) {
            ArgumentInfo arg = info.arg_help[j];
            os << arg.kwd;
            if (!info.arg_help[j].default_value.empty()) {
                os << "=" << info.arg_help[j].default_value;
            } else if (!info.arg_help[j].mandatory) {
                os << "=None";
            }
            if (j != info.arg_help.size() - 1)
                os << ", ";
        }
        os << ")" << endl;
    }
}

void PlainPrinter::print_arguments(const DocStruct &info) {
    for (size_t j(0); j != info.arg_help.size(); ++j) {
        ArgumentInfo arg = info.arg_help[j];
        os << " " << arg.kwd << "("
           << arg.type_name << "): "
           << arg.help << endl;
    }
}

void PlainPrinter::print_notes(const DocStruct &info) {
    if (print_all) {
        for (size_t j(0); j != info.notes.size(); ++j) {
            NoteInfo note = info.notes[j];
            if (note.long_text) {
                os << "=== " << note.name << " ===" << endl
                   << note.description << endl << endl;
            } else {
                os << " * " << note.name << ": " << note.description << endl << endl;
            }
        }
    }
}

void PlainPrinter::print_language_features(const DocStruct &info) {
    if (print_all) {
        if (!info.support_help.empty()) {
            os << "Language features supported:" << endl;
        }
        for (size_t j(0); j != info.support_help.size(); ++j) {
            LanguageSupportInfo ls = info.support_help[j];
            os << " * " << ls.feature << ": " << ls.description << endl;
        }
    }
}

void PlainPrinter::print_properties(const DocStruct &info) {
    if (print_all) {
        if (!info.property_help.empty()) {
            os << "Properties:" << endl;
        }
        for (size_t j(0); j != info.property_help.size(); ++j) {
            PropertyInfo p = info.property_help[j];
            os << " * " << p.property << ": " << p.description << endl;
        }
    }
}


void PlainPrinter::print_category_header(string category_name) {
    os << "Help for " << category_name << endl << endl;
}

void PlainPrinter::print_category_footer() {
    os << endl;
}
