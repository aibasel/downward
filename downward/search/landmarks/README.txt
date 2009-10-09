This is the README file for the landmarks code.

There are 2 main classes here: 
	LandmarksCountHeuristic is the landmarks heuristic class.
	LandmarksGraph is the class that represents the landmarks and orderings.
		All the classes that inherit from LandmarksGraph implement different methods of
		landmarks extraction.

In addition, there are 2 helper classes:
	LandmarkStatusManager is a helper class that deals with managing the status of 
		landmarks at each state. It contains a hash map from states to set of reached
		landmarks.
	LandmarkCostAssignment is a class that does cost partioning for landmarks, 
		in order to get an admissible heuristic. It has two subclasses that
		implement uniform and optimal LP-based cost sharing.
		
		
util.h/cc contains some code that does not belong in any class	