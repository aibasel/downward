#include "a_star_search.h"

#include "globals.h"
#include "heuristic.h"
#include "successor_generator.h"

#include <cassert>
#include <cstdlib>
using namespace std;

AStarSearchEngine::AStarSearchEngine():
	GeneralEagerBestFirstSearch(1, 1, GeneralEagerBestFirstSearch::h, true)  {
}

AStarSearchEngine::~AStarSearchEngine() {
}
