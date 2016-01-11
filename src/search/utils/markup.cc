#include "markup.h"

#include <sstream>

using namespace std;

namespace Utils {
static string t2t_escape(const std::string &s) {
    return "\"\"" + s + "\"\"";
}

string format_paper_reference(
    const string &authors, const string &title, const string &url,
    const string &conference, const string &pages, const string &publisher) {
    stringstream ss;
    ss << "\n\n"
       << " * " << t2t_escape(authors) << ".<<BR>>\n"
       << " [" << t2t_escape(title) << " " << t2t_escape(url) << "].<<BR>>\n"
       << " In //" << t2t_escape(conference) << "//,"
       << " pp. " << t2t_escape(pages) << ". "
       << t2t_escape(publisher) << ".\n\n\n";
    return ss.str();
}

}
