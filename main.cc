//
// author : Toru Shiozaki
// date   : Jan 2012
//

// This program is supposed to perform Wick's theorem for multireference problems.
// Spin averaged quantities assumed.

#include <iostream>
#include <list>
#include "equation.h"
#include "tree.h"

using namespace std;

int main() {

  shared_ptr<Op> proj(new Op("proj", "c", "c", "a", "a"));
  shared_ptr<Op> f(new Op("f1", "g", "g"));
  shared_ptr<Op> t(new Op("t2", "a", "a", "c", "c"));
  shared_ptr<Op> H(new Op("H", "g", "g", "g", "g"));

  list<shared_ptr<Op> > d;
  d.push_back(proj);
#if 1
  d.push_back(f);
  d.push_back(t);
#else
  d.push_back(H);
#endif

  shared_ptr<Diagram> di(new Diagram(d));
  shared_ptr<Equation> eq(new Equation(di, "MP2"));
  eq->duplicates();
  eq->active();

  shared_ptr<Tree> res(new Tree(eq));
  res->print();

  cout << "-----" << endl;
  cout << res->generate_task_list() << endl;
  cout << "-----" << endl;

  cout << res->generate() << endl;
}
