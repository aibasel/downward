#include <vector>
#include <ext/hash_map>

#include "landmarks_graph_rpg_exhaust.h"
#include "../state.h"

/* Problem: We don't get any orders here. (All we have is the reasonable orders
   that are inferred later.) It's thus best to combine this landmark generation
   method with others, don't use it by itself. */

void LandmarksGraphExhaust::generate_landmarks() {
    cout << "Generating landmarks by testing all facts with RPG method" << endl;

    // insert goal landmarks and mark them as goals
    for(unsigned i = 0; i < g_goal.size(); i++) {
	LandmarkNode* lmp = &landmark_add_simple(g_goal[i]);
	lmp->in_goal = true;
    }
    // test all other possible facts
    for(int i = 0; i < g_variable_name.size(); i++)
	for(int j = 0; j < g_variable_domain[i]; j++) {
	    const pair<int, int> lm = make_pair(i,j);
	    if(!simple_landmark_exists(lm)) {
		LandmarkNode* new_lm = &(landmark_add_simple(lm));
		if((*g_initial_state)[lm.first] != lm.second && relaxed_task_solvable(true, new_lm))
		    rm_landmark(lm);
	    }
	}
}
