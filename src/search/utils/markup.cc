#include "markup.h"

#include <cassert>
#include <sstream>

using namespace std;

namespace utils {
static string t2t_escape(const string &s) {
    return "\"\"" + s + "\"\"";
}

static string format_authors(const vector<string> &authors) {
    assert(!authors.empty());
    int num_authors = authors.size();
    stringstream ss;
    for (int i = 0; i < num_authors; ++i) {
        const string &author = authors[i];
        ss << t2t_escape(author);
        if (i < num_authors - 2) {
            ss << ", ";
        } else if (i == num_authors - 2) {
            ss << " and ";
        }
    }
    return ss.str();
}

string format_paper_reference(
    const vector<string> &authors, const string &title, const string &url,
    const string &conference, const string &pages, const string &publisher) {
    stringstream ss;
    ss << "\n\n"
       << " * " << format_authors(authors) << ".<<BR>>\n"
       << " [" << t2t_escape(title) << " " << url << "].<<BR>>\n"
       << " In //" << t2t_escape(conference) << "//,"
       << " pp. " << t2t_escape(pages) << ". "
       << t2t_escape(publisher) << ".\n\n\n";
    return ss.str();
}
}
