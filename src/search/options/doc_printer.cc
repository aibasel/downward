#include "doc_printer.h"

#include "doc_store.h"

#include <iostream>

using namespace std;

namespace options {
static bool is_call(string s) {
    return s.find("(") != string::npos;
}

DocPrinter::DocPrinter(ostream &out)
    : os(out) {
}

DocPrinter::~DocPrinter() {
}

void DocPrinter::print_all() {
    DocStore *ds = DocStore::instance();
    vector<string> types = ds->get_types();
    for (size_t n = 0; n < types.size(); ++n) {
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
    for (size_t i = 0; i < keys.size(); ++i) {
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
        for (size_t i = 0; i < info.arg_help.size(); ++i) {
            ArgumentInfo arg = info.arg_help[i];
            os << arg.kwd;
            if (!arg.default_value.empty())
                os << "=" << arg.default_value;
            if (i != info.arg_help.size() - 1)
                os << ", ";
        }
        os << ")" << endl << endl << endl;
    }
}

void Txt2TagsPrinter::print_arguments(const DocStruct &info) {
    for (size_t i = 0; i < info.arg_help.size(); ++i) {
        ArgumentInfo arg = info.arg_help[i];
        os << "- //" << arg.kwd << "// (" << arg.type_name;
        if (arg.bounds.has_bound())
            os << " \"\"" << arg.bounds << "\"\"";
        os << "): " << arg.help << endl;
        if (!arg.value_explanations.empty()) {
            for (size_t j = 0; j < arg.value_explanations.size(); ++j) {
                pair<string, string> explanation =
                    arg.value_explanations[j];
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
    for (size_t i = 0; i < info.notes.size(); ++i) {
        NoteInfo note = info.notes[i];
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
    for (size_t i = 0; i < info.support_help.size(); ++i) {
        LanguageSupportInfo ls = info.support_help[i];
        os << "- **" << ls.feature << ":** " << ls.description << endl;
    }
}

void Txt2TagsPrinter::print_properties(const DocStruct &info) {
    if (!info.property_help.empty()) {
        os << "Properties:" << endl;
    }
    for (size_t i = 0; i < info.property_help.size(); ++i) {
        PropertyInfo p = info.property_help[i];
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
        for (size_t i = 0; i < info.arg_help.size(); ++i) {
            ArgumentInfo arg = info.arg_help[i];
            os << arg.kwd;
            if (!arg.default_value.empty())
                os << "=" << arg.default_value;
            if (i != info.arg_help.size() - 1)
                os << ", ";
        }
        os << ")" << endl;
    }
}

void PlainPrinter::print_arguments(const DocStruct &info) {
    for (size_t i = 0; i < info.arg_help.size(); ++i) {
        ArgumentInfo arg = info.arg_help[i];
        os << " " << arg.kwd << " (" << arg.type_name;
        if (arg.bounds.has_bound())
            os << " " << arg.bounds;
        os << "): " << arg.help << endl;
    }
}

void PlainPrinter::print_notes(const DocStruct &info) {
    if (print_all) {
        for (size_t i = 0; i < info.notes.size(); ++i) {
            NoteInfo note = info.notes[i];
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
        for (size_t i = 0; i < info.support_help.size(); ++i) {
            LanguageSupportInfo ls = info.support_help[i];
            os << " * " << ls.feature << ": " << ls.description << endl;
        }
    }
}

void PlainPrinter::print_properties(const DocStruct &info) {
    if (print_all) {
        if (!info.property_help.empty()) {
            os << "Properties:" << endl;
        }
        for (size_t i = 0; i < info.property_help.size(); ++i) {
            PropertyInfo p = info.property_help[i];
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
}
