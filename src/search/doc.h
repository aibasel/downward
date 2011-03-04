/*! \mainpage Fast Downward Index Page
 *
 * \section brief_sec Brief Description
 *
 * Following is a brief description of the general structure of Fast Downward:
 *
 * Fast Downward is a heuristic search progression planner, using the SAS+ formalism.
 * Many heuristics have been implemented in Fast Downward, and they will not be described here.
 * This section describes the main classes involved with search in Fast Downward.
 *
 * \subsection search_engine_sec Search Engine
 * The SearchEngine class is the base class for all search algorithms.
 * The method search() of SearchEngine performs the actual search, by repeatedly calling
 * the virtual method step() until a solution is found, or the search space is exhausted.
 *
 * There are two main implementations of SearchEngine:
 * - EagerSearch - implements eager search (i.e. each
 * state and its heuristic value is computed before inserting into the open list)
 * - LazySearch - implements lazy search (i.e. each
 * state and its heuristic value is computed only when removed from the open list)
 *
 * Each of these main classes can use a generic open list to support using multiple heuristics
 * in various ways. They have been subclasses to create a simple way to use some
 * standard search algorithms, like A* (AStarSearchEngine).
 *
 * \subsection open_list_sec Open List
 *
 * The OpenList class is the base class for all open lists.
 * In Fast Downward, the open list (and not the search engine) is responsible for
 * handling multiple heuristics and preferred operators in various ways.
 *
 * - StandardScalarOpenList - implements a standard (one value) open list
 * - TieBreakingOpenList - implements a standard tie breaking open list (i.e. value1 is more important than value2)
 * - ParetoOpenList - implements pareto-optimal open lists (i.e. the next node to be removed is pareto optimal according to all values)
 * - AlternationOpenList - implements the dual queues approach
 *
 * Most open lists use Evaluator objects (StandardScalarOpenList has one Evaluator,
 * TieBreakingOpenList and ParetoOpenList have several Evaluators).
 * AlternationOpenList uses several other OpenList objects, each of which contain their
 * own Evaluators.
 *
 *
 * \subsection evaluator_sec Evaluators and Heuristics
 *
 * Evaluator objects (which generalize heuristics) are objects which evaluate a state, and
 * are used by open lists. Currently, the only supported Evaluator is a ScalarEvaluator, which
 * returns a single numeric value.
 *
 * The implemented ScalarEvaluators are:
 * - Heuristic - any heuristic is an evaluator, where the evaluation is the heuristic value
 * - GEvaluator - the evaluation of a state is simply the g-value of a state
 * - SumEvaluator - the evaluation of a state is the sum of several internal evaluators.
 * This can be used to create f=g+h where g is a GEvaluator, and h is a Heuristic.
 * - WeightedEvaluator - the evaluation of a state is weighted by a constant factor.
 * This can be used in weighted A* (i.e. f=g+wh)
 *
 */
