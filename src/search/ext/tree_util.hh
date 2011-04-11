/* 

	A collection of miscellaneous utilities that operate on the templated 
	tree.hh class.


	Copyright (C) 2001-2009  Kasper Peeters <kasper.peeters@aei.mpg.de>

	(At the moment this only contains a printing utility, thanks to Linda
	Buisman <linda.buisman@studentmail.newcastle.edu.au>)

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#ifndef tree_util_hh_
#define tree_util_hh_

#include <iostream>
#include "tree.hh"

namespace kptree {

template<class T>
void print_tree_bracketed(const tree<T>& t, std::ostream& str=std::cout);

template<class T>
void print_subtree_bracketed(const tree<T>& t, typename tree<T>::iterator iRoot, 
									  std::ostream& str=std::cout);



// Iterate over all roots (the head) and print each one on a new line
// by calling printSingleRoot.

template<class T>
void print_tree_bracketed(const tree<T>& t, std::ostream& str) 
	{
	int headCount = t.number_of_siblings(t.begin());
	int headNum = 0;
	for(typename tree<T>::sibling_iterator iRoots = t.begin(); iRoots != t.end(); ++iRoots, ++headNum) {
		print_subtree_bracketed(t,iRoots,str);
		if (headNum != headCount) {
			str << std::endl;
			}
		}
	}


// Print everything under this root in a flat, bracketed structure.

template<class T>
void print_subtree_bracketed(const tree<T>& t, typename tree<T>::iterator iRoot, std::ostream& str) 
	{
	if(t.empty()) return;
	if (t.number_of_children(iRoot) == 0) {
		str << *iRoot;	
		}
	else {
		// parent
		str << *iRoot;
		str << "(";
		// child1, ..., childn
		int siblingCount = t.number_of_siblings(t.begin(iRoot));
		int siblingNum;
		typename tree<T>::sibling_iterator iChildren;
		for (iChildren = t.begin(iRoot), siblingNum = 0; iChildren != t.end(iRoot); ++iChildren, ++siblingNum) {
			// recursively print child
			print_subtree_bracketed(t,iChildren,str);
			// comma after every child except the last one
			if (siblingNum != siblingCount ) {
				str << ", ";
				}
			}
		str << ")";
		}
	}

}

#endif
