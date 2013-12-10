#include "label.h"

#include "../globals.h"

#include <ostream>

using namespace std;

Label::Label(int index_, int cost_, const vector<Prevail> &prevail_, const vector<PrePost> &pre_post_)
    : index(index_), cost(cost_), prevail(prevail_), pre_post(pre_post_) {
    marker1 = marker2 = false;
}

Label::Label(int index_, const Label *label)
    : index(index_), cost(label->get_cost()), prevail(label->get_prevail()),
      pre_post(label->get_pre_post()) {
    marker1 = marker2 = false;
    add_mapping_label(label);
}

void Label::add_mapping_label(const Label *label) const {
    assert(cost == label->get_cost());
    // TODO: check that prevail/pre_post are indeed "local equivalent"?
    mapping_labels.push_back(label);
}

void Label::dump() const {
    cout << "index: " << index << (index < g_operators.size() ? " regular operator" : "" ) << endl;
    cout << "cost: " << cost << endl;
    /*if (canonical_op) {
        cout << "canonical op:" << endl;
        canonical_op->dump();
        cout << "cost: " << canonical_op->get_cost() << endl;
    }*/
}
