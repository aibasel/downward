#include "label.h"

#include "../globals.h"

#include <ostream>

using namespace std;

Label::Label(int id_, int cost_, const vector<Prevail> &prevail_, const vector<PrePost> &pre_post_)
    : id(id_), cost(cost_), prevail(prevail_), pre_post(pre_post_), root(0) {
    marker1 = marker2 = false;
}

SingleLabel::SingleLabel(int id, int cost, const vector<Prevail> &prevail, const vector<PrePost> &pre_post)
    : Label(id, cost, prevail, pre_post) {
}

CompositeLabel::CompositeLabel(int id, const std::vector<const Label *> &labels)
    : Label(id, labels[0]->get_cost(), labels[0]->get_prevail(), labels[0]->get_pre_post()),
      parents(labels) {
    // TODO: here we take the first label as the "canonical" label for prevail and
    // pre-post conditions. can we somehow check the other labels in labels for
    // local equivalence?
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
