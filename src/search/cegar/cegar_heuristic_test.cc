#include <gtest/gtest.h>

#include "./cegar_heuristic.h"
#include "./../operator.h"

namespace cegar_heuristic {

TEST(CegarTest, regress) {
    AbstractState a = AbstractState();
    vector<string> prevail;
    prevail.push_back("1 0");
    vector<string> pre_post;
    Operator op = create_op("op", prevail, pre_post);
    ASSERT_EQ(16, 4*4);
}

}
