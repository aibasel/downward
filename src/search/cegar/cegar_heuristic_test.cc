#include <gtest/gtest.h>
#include <vector>
#include <set>

#include "./cegar_heuristic.h"

using namespace std;
using namespace __gnu_cxx;

namespace cegar_heuristic {

TEST(CegarTest, aha) {
    AbstractState a = AbstractState();
    ASSERT_EQ(16, 4*4);
}

}
