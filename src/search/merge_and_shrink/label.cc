#include "label.h"

#include "../utilities.h"
#include "../globals.h"

#include <ostream>

using namespace std;

Label::Label(int id_, int cost_, const vector<Prevail> &prevail_, const vector<PrePost> &pre_post_)
    : id(id_), cost(cost_), prevail(prevail_), pre_post(pre_post_), root(this) {
}

bool Label::is_reduced() const {
    return root != this;
}

OperatorLabel::OperatorLabel(int id, int cost, const vector<Prevail> &prevail,
                             const vector<PrePost> &pre_post)
    : Label(id, cost, prevail, pre_post) {
}

CompositeLabel::CompositeLabel(int id, const std::vector<Label *> &parents_)
    : Label(id, parents_[0]->get_cost(), parents_[0]->get_prevail(), parents_[0]->get_pre_post()) {
    // We take the first label as the "canonical" label for prevail and pre-post
    // to match the old implementation of label reduction.
    for (size_t i = 0; i < parents_.size(); ++i) {
        Label *parent = parents_[i];
        if (i > 0)
            assert(parent->get_cost() == parents_[i - 1]->get_cost());
        parent->update_root(this);
        parents.push_back(parent);
    }
}

void OperatorLabel::update_root(CompositeLabel *new_root) {
    root = new_root;
}

void CompositeLabel::update_root(CompositeLabel *new_root) {
    for (size_t i = 0; i < parents.size(); ++i)
        parents[i]->update_root(new_root);
    root = new_root;
}

const std::vector<Label *> &OperatorLabel::get_parents() const {
    exit_with(EXIT_CRITICAL_ERROR);
}

void Label::dump() const {
    cout << id << "->" << root->get_id() << endl;
    //cout << "index: " << id << (id < g_operators.size() ? " regular operator" : "" ) << endl;
    //cout << "cost: " << cost << endl;
}
