#include "label.h"

#include "../globals.h"

#include <ostream>

using namespace std;

Label::Label(int id_, int cost_, const vector<Prevail> &prevail_, const vector<PrePost> &pre_post_)
    : id(id_), cost(cost_), prevail(prevail_), pre_post(pre_post_) {
    marker1 = marker2 = false;
}

SingleLabel::SingleLabel(int id, int cost, const vector<Prevail> &prevail, const vector<PrePost> &pre_post)
    : Label(id, cost, prevail, pre_post) {
}

CompositeLable::CompositeLable(int id, const Label *label)
    : Label(id, label->get_cost(), label->get_prevail(), label->get_pre_post()) {
    add_mapping_label(label);
}

void Label::add_mapping_label(const Label *label) const {
    assert(cost == label->get_cost());
    // TODO: check that prevail/pre_post are indeed "local equivalent"?
    mapping_labels.push_back(label);
}

void Label::dump() const {
    cout << "index: " << id << (id < g_operators.size() ? " regular operator" : "" ) << endl;
    cout << "cost: " << cost << endl;
    /*if (canonical_op) {
        cout << "canonical op:" << endl;
        canonical_op->dump();
        cout << "cost: " << canonical_op->get_cost() << endl;
    }*/
}
