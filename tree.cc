//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: tree.cc
// Copyright (C) 2012 Toru Shiozaki
//
// Author: Toru Shiozaki <shiozaki@northwestern.edu>
// Maintainer: Shiozaki group
//
// This file is part of the SMITH3 package.
//
// The SMITH3 package is free software; you can redistribute it and\/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SMITH3 package is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SMITH3 package; see COPYING.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//


#include "tree.h"
#include "energy.h"
#include "correction.h"
#include "residual.h"
#include "density.h"
#include "constants.h"
#include <algorithm>
#include <stdexcept>
#include <map>

using namespace std;
using namespace smith;


Tree::Tree(shared_ptr<Equation> eq, string lab) : parent_(NULL), tree_name_(eq->name()), num_(-1), label_(lab) {
  // First make ListTensor for all the diagrams
  list<shared_ptr<Diagram>> d = eq->diagram();

  for (auto i = d.begin(); i != d.end(); ++i) {
    shared_ptr<ListTensor> tmp(new ListTensor(*i));
    // All internal tensor should be included in the active part
    tmp->absorb_all_internal();   

    shared_ptr<Tensor> first = tmp->front();
    shared_ptr<ListTensor> rest = tmp->rest();

    // convert to tree and then bc
    if (label_ == "residual") { 
      shared_ptr<Tree> tr(new Residual(rest, lab));
      list<shared_ptr<Tree>> lt; lt.push_back(tr);
      shared_ptr<BinaryContraction> b(new BinaryContraction(lt, first, (*i)->ex_target_index()));
      bc_.push_back(b);
    } else if (label_ == "energy") {
      shared_ptr<Tree> tr(new Energy(rest, lab));
      list<shared_ptr<Tree>> lt; lt.push_back(tr);
      shared_ptr<BinaryContraction> b(new BinaryContraction(lt, first, (*i)->ex_target_index()));
      bc_.push_back(b);
    } else if (label_ == "correction") {
      shared_ptr<Tree> tr(new Correction(rest, lab));
      list<shared_ptr<Tree>> lt; lt.push_back(tr);
      shared_ptr<BinaryContraction> b(new BinaryContraction(lt, first, (*i)->ex_target_index()));
      bc_.push_back(b);
    } else if (label_ == "density") {
      shared_ptr<Tree> tr(new Density(rest, lab));
      list<shared_ptr<Tree>> lt; lt.push_back(tr);
      shared_ptr<BinaryContraction> b(new BinaryContraction(lt, first, (*i)->ex_target_index()));
      bc_.push_back(b);
    } else {
      throw logic_error("Error Tree::Tree, code generation for this tree type not implemented");
    }
  
  }

  factorize();
  set_parent_sub();
  set_target_rec();
}


Tree::Tree(const shared_ptr<ListTensor> l, string lab) : num_(-1), label_(lab) {
  target_ = l->target();
  dagger_ = l->dagger();
  if (l->length() > 1) {
    shared_ptr<BinaryContraction> bc(new BinaryContraction(target_, l, lab));
    bc_.push_back(bc);
  } else {
    shared_ptr<Tensor> t = l->front();
    t->set_factor(l->fac());
    t->set_scalar(l->scalar());
    op_.push_back(t);
  }
}


BinaryContraction::BinaryContraction(shared_ptr<Tensor> o, shared_ptr<ListTensor> l, string lab) : label_(lab) {
  // o is a target tensor NOT included in l
  target_ = o;
  tensor_ = l->front();
  shared_ptr<ListTensor> rest = l->rest();
  
  // convert to correct subtree
  if (label_ == "residual") { 
    shared_ptr<Tree> tr(new Residual(rest, lab));
    subtree_.push_back(tr);
  } else if (label_ == "energy") {
    shared_ptr<Tree> tr(new Energy(rest, lab));
    subtree_.push_back(tr);
  } else if (label_ == "correction") {
    shared_ptr<Tree> tr(new Correction(rest, lab));
    subtree_.push_back(tr);
  } else if (label_ == "density") {
    shared_ptr<Tree> tr(new Density(rest, lab));
    subtree_.push_back(tr);
  } else {
    throw logic_error("Error BinaryContraction::BinaryContraction, code generation for this tree type not implemented");
  }

}


void Tree::factorize() {
  list<list<shared_ptr<BinaryContraction>>::iterator> done;
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
    if (find(done.begin(), done.end(), i) != done.end()) continue;
    auto j = i; ++j;
    for ( ; j != bc_.end(); ++j) {
      // bc is factorized according to excitation target indices
      if (*(*i)->tensor() == *(*j)->tensor() && (*i)->dagger() == (*j)->dagger() && (*i)->ex_target_index_str() == (*j)->ex_target_index_str()) {
        done.push_back(j);
        (*i)->subtree().insert((*i)->subtree().end(), (*j)->subtree().begin(), (*j)->subtree().end());
      }
    }
  }
  for (auto& i : done) bc_.erase(i);
  for (auto& i : bc_) i->factorize();
}


void BinaryContraction::factorize() {
  list<list<shared_ptr<Tree>>::iterator> done;
  // and then merge subtrees
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) {
    if (find(done.begin(), done.end(), i) != done.end()) continue;
    auto j = i; ++j;
    for ( ; j != subtree_.end(); ++j) {
      if (find(done.begin(), done.end(), j) != done.end()) continue;
      if ((*i)->merge(*j)) done.push_back(j);
    }
  }
  for (auto i = done.begin(); i != done.end(); ++i) subtree_.erase(*i);
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) (*i)->factorize();
}
  

bool Tree::merge(shared_ptr<Tree> o) {
  bool out = false;
  if (o->bc_.size() > 0) {
    shared_ptr<Tensor> a = bc_.front()->tensor();
    shared_ptr<Tensor> b = o->bc_.front()->tensor();
    if (*a == *b) {
      out = true;
      bc_.insert(bc_.end(), o->bc_.begin(), o->bc_.end());
      for (auto i = bc_.begin(); i != bc_.end(); ++i) (*i)->set_target(target_);
    }
  } else if (o->op_.size() > 0) {
    out = true;
    op_.insert(op_.end(), o->op_.begin(), o->op_.end());
  }
  return out;
}


void BinaryContraction::set_target_rec() {
  if (!subtree_.empty()) {
    auto i = subtree_.begin();
    shared_ptr<Tensor> first = (*i)->target();
    for (++i ; i != subtree_.end(); ++i) {
      (*i)->set_target(first);
    }
    for (i = subtree_.begin() ; i != subtree_.end(); ++i)
      (*i)->set_target_rec();
  }
}


void Tree::set_target_rec() {
  if (!bc_.empty()) {
    for (auto& i : bc_) i->set_target_rec();
  }
}


void BinaryContraction::set_parent_subtree() {
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) {
    (*i)->set_parent(this);
    (*i)->set_parent_sub();
  }
}


void Tree::set_parent_sub() {
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
    (*i)->set_parent(this);
    (*i)->set_parent_subtree();
  }
}


int BinaryContraction::depth() const { return parent_->depth(); }

int Tree::depth() const { return parent_ ? parent_->parent()->depth()+1 : 0; }


bool BinaryContraction::dagger() const {
  return subtree_.front()->dagger();
}


shared_ptr<Tensor> BinaryContraction::next_target() {
  return subtree().front()->target();
}


void Tree::sort_gamma(std::list<std::shared_ptr<Tensor>> o) {
  gamma_ = o;

  list<shared_ptr<Tensor>> g = gather_gamma();
  for (auto& i : g) find_gamma(i);
}


void Tree::find_gamma(shared_ptr<Tensor> o) {
  bool found = false;
  for (auto& i : gamma_) {
    if ((*i) == (*o)) {
      found = true;
      o->set_alias(i);
      break;
    }
  }
  if (!found) gamma_.push_back(o);
}


// return gamma below this
list<shared_ptr<Tensor>> Tree::gather_gamma() const {
  list<shared_ptr<Tensor>> out;
  for (auto& i : bc_) {
    for (auto& j : i->subtree()) {
      // recursive call 
      list<shared_ptr<Tensor>> tmp = j->gather_gamma();
      out.insert(out.end(), tmp.begin(), tmp.end());
    }
  }
  for (auto& i : op_) 
    if (i->label().find("Gamma") != string::npos) out.push_back(i); 
  return out;
}


string BinaryContraction::ex_target_index_str() const { 
  stringstream zz;
  if (!ex_target_index_.empty()) {
    zz << "(";
    for (auto i = ex_target_index_.begin(); i != ex_target_index_.end(); ++i) {
      if (i != ex_target_index_.begin()) zz << ", ";
      zz << (*i)->str(false);
    }
    zz << ")";
  } 
  return zz.str(); 
}


void BinaryContraction::print() const {
  string indent = "";
  for (int i = 0; i != depth(); ++i) indent += "  ";
  cout << indent << (target_ ? (target_->str() + " = ") : "") << tensor_->str() << (depth() == 0 ? ex_target_index_str() : "") << " * " << subtree_.front()->target()->str() << endl;
  for (auto& i : subtree_) i->print();
}


void Tree::print() const {
  string indent = "";
  for (int i = 0; i != depth(); ++i) indent += "  ";
  if (target_) {
    for (auto i = op_.begin(); i != op_.end(); ++i)
      cout << indent << target_->str() << " += " << (*i)->str() << (dagger_ ? " *" : "") << endl;
  }
  for (auto i = bc_.begin(); i != bc_.end(); ++i)
    (*i)->print();
}


static int icnt;
static int i0;
// not a good implementation...
vector<shared_ptr<Tensor>> ii;

// local functions... (not a good practice...) >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
static string label__(const string& lbl) {
  string label = lbl;
  if (label == "proj") label = "r";
  size_t found = label.find("dagger");
  if (found != string::npos) {
    string tmp(label.begin(), label.begin() + found);
    label = tmp;
  }
  return label;
}
// local functions... (not a good practice...) <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



list<shared_ptr<const Index>> BinaryContraction::target_indices() {
  // returns a list of target indices
  return target_->index();
}


list<shared_ptr<const Index>> BinaryContraction::loop_indices() {
  // returns a list of inner loop indices.
  list<shared_ptr<const Index>> out;
  list<shared_ptr<const Index>> ti = target_->index();
  for (auto iter = tensor_->index().begin(); iter != tensor_->index().end(); ++iter) {
    bool found = false;
    for (auto i = ti.begin(); i != ti.end(); ++i) {
      if ((*i)->identical(*iter)) {
        found = true;
        break;
      }
    }
    if (!found) {
      out.push_back(*iter);
    }
  }
  return out;
}


string Tree::generate_compute_operators(const string indent, const shared_ptr<Tensor> target, const vector<shared_ptr<Tensor>> op,
                                        const bool dagger) const {
  stringstream tt;

  vector<string> close;
  string cindent = indent + "    ";
  // note that I am using reverse_iterator
  //tt << target->generate_loop(cindent, close);
  // get data
  tt << target->generate_get_block(cindent, "o", "out()", true);
  
  // needed in case the tensor labels are repeated..
  vector<shared_ptr<Tensor>> uniq_tensors;
  vector<string> tensor_labels;
  // map redundant tensor label to unique tensor label
  map<string, int> op_tensor_lab;

  int uniq_cnt = 0;
  for (auto& i : op) {
    string label = label__(i->label());
    // ignore gamma or intermediate tensors TODO find better alternative to this code
    if (label.find("Gamma") != string::npos) continue;
    if (label.find("I") != string::npos) continue;
    // if we already have the label in the list
    if (find(tensor_labels.begin(), tensor_labels.end(), label) != tensor_labels.end()) {
      continue;
    }
    // otherwise not yet found, so add to list
    tensor_labels.push_back(label);
    uniq_tensors.push_back(i);
    op_tensor_lab[label] = uniq_cnt;
    ++uniq_cnt;
  }


  // add the source data to the target
  int j = 0;
  for (auto s = op.begin(); s != op.end(); ++s, ++j) {
    stringstream uu; uu << "i" << j;
    tt << cindent << "{" << endl;

    // uses map to give label number consistent with operator, needed in case label is repeated (eg ccaa)
    string label = label__((*s)->label());
    stringstream instr; instr << "in(" << op_tensor_lab[label] << ")";

    tt << (*s)->generate_get_block(cindent+"  ", uu.str(), instr.str());
    list<shared_ptr<const Index>> di = target->index();
    tt << (*s)->generate_sort_indices(cindent+"  ", uu.str(), instr.str(), di, true);
    tt << cindent << "}" << endl;

    // if this is at the top-level and needs to be daggered:
    if (depth() == 0 && dagger) {
      shared_ptr<Tensor> top(new Tensor(**s));
      // swap operators so that tensor is daggered
      if (top->index().size() != 4) {
         throw logic_error("Daggered object is only supported for 4-index tensors");
      } else {
        auto k0 = di.begin(); auto k1 = k0; ++k1; auto k2 = k1; ++k2; auto k3 = k2; ++k3;
        list<pair<list<shared_ptr<const Index>>::iterator, list<shared_ptr<const Index>>::iterator>> map;
        map.push_back(make_pair(k0, k2));
        map.push_back(make_pair(k2, k0));
        map.push_back(make_pair(k1, k3));
        map.push_back(make_pair(k3, k1));
        list<shared_ptr<const Index>> tmp;
        for (auto& k : top->index()) {
          for (auto l = map.begin(); l != map.end(); ++l) {
            if (k->identical(*l->first)) {
              tmp.push_back(*l->second);
              break;
            }
            auto ll = l;
            if (++ll == map.end()) throw logic_error("should not happen: dagger stuffs");
          }
        }
        top->index() = tmp;
        tt << cindent << "{" << endl;
        tt << top->generate_get_block(cindent+"  ", uu.str(), instr.str());
        list<shared_ptr<const Index>> di = target->index();
        tt << top->generate_sort_indices(cindent+"  ", uu.str(), instr.str(), di, true);
        tt << cindent << "}" << endl;
      }
    }
  }

  // put data
  {
    string label = target->label();
    list<shared_ptr<const Index>> ti = target->index();
    tt << cindent << "out()->put_block(odata";
    for (auto i = ti.rbegin(); i != ti.rend(); ++i) 
      tt << ", " << (*i)->str_gen();
    tt << ");" << endl;
  }
  for (auto iter = close.rbegin(); iter != close.rend(); ++iter)
        tt << *iter << endl;
  return tt.str();
}


string Tree::generate_task(const string indent, const int ic, const vector<shared_ptr<Tensor>> op, const int iz) const {
  stringstream ss;

  vector<string> ops;
  for (auto& i : op) ops.push_back(i->label());
  int ip = -1;
  if (parent_) ip = parent_->parent()->num();

  string scalar;
  for (auto& i : op) {
    if (!i->scalar().empty()) {
      if (i->scalar() != "e0") throw logic_error("unknown scalar. Tree::generate_task");
      if (!scalar.empty() && i->scalar() != scalar) throw logic_error("multiple scalars. Tree::generate_task");
      scalar = i->scalar(); 
    }
  }

  // if gamma, we need to add dependency. We need to go up to the root tree. 
  ss << generate_task(indent, ip, ic, ops, scalar, iz);
  for (auto& i : op) {
    if (i->label().find("Gamma") != string::npos)
      ss << indent << "task" << ic << "->" << add_depend(i) << endl;
  }
  ss << endl;

  return ss.str();
}


string Tree::add_depend(const std::shared_ptr<const Tensor> o) const {
  stringstream out;
  if (!parent_) {
    assert(!gamma_.empty());
    for (auto& i : gamma_) {
      if (o->label() == i->label()) out << "add_dep(task" << i->num() << ");"; 
    }
  } else {
    out << parent_->parent()->add_depend(o);
  }
  return out.str();
}


pair<string, string> BinaryContraction::generate_task_list() const {
  stringstream ss, tt;
  for (auto& i : subtree_) {
    pair<string, string> tmp = i->generate_task_list();
    ss << tmp.first;
    tt << tmp.second;
  }
  return make_pair(ss.str(), tt.str());
}


pair<string, string> Tree::generate_task_list(const list<shared_ptr<Tree>> tree_list) const {
  // here ss is the Method.h driver routine
  stringstream ss;
  // here tt is the Method_tasks.h driver routine
  stringstream tt;
  
  string indent = "      ";
  // if this is the top tree, we want to initialize index, as well as to create a task that zeros out the residual vector
  if (depth() == 0) {
    assert(icnt == 0 && tree_name_ != "");

    // save task zero
    i0 = icnt;

    ss << header(tree_name_);
    tt << header(tree_name_ + "_tasks");

    ss << "#ifndef __SRC_SMITH_" << tree_name_ << "_H " << endl;
    ss << "#define __SRC_SMITH_" << tree_name_ << "_H " << endl;
    ss << "" << endl;
    ss << "#include <src/smith/spinfreebase.h>" << endl;
    ss << "#include <src/scf/fock.h>" << endl;
    ss << "#include <src/util/f77.h>" << endl;
    ss << "#include <iostream>" << endl;
    ss << "#include <tuple>" << endl;
    ss << "#include <iomanip>" << endl;
    ss << "#include <src/smith/queue.h>" << endl;
    ss << "#include <src/smith/" << tree_name_ << "_tasks.h>" << endl;
    ss << "#include <src/smith/smith_info.h>" << endl;
    ss << "" << endl;
    ss << "namespace bagel {" << endl;
    ss << "namespace SMITH {" << endl;
    ss << "namespace " << tree_name_ << "{" << endl;
    ss << "" << endl;
    ss << "template <typename T>" << endl;
    ss << "class " << tree_name_ << " : public SpinFreeMethod<T>, SMITH_info {" << endl;
    ss << "  protected:" << endl;
    ss << "    std::shared_ptr<Tensor<T>> t2;" << endl;
    ss << "    std::shared_ptr<Tensor<T>> r;" << endl;
    ss << "    std::shared_ptr<Tensor<T>> d;" << endl;
    ss << "    double e0_;" << endl;
    ss << "" << endl;
    ss << "    std::tuple<std::shared_ptr<Queue<T>>, std::shared_ptr<Queue<T>>,  std::shared_ptr<Queue<T>>,  std::shared_ptr<Queue<T>>> make_queue_() {" << endl;
    ss << "      std::shared_ptr<Queue<T>> queue_(new Queue<T>());" << endl;
    ss << indent << "std::array<std::shared_ptr<const IndexRange>,3> pindex = {{this->rclosed_, this->ractive_, this->rvirt_}};" << endl << endl;

    tt << "#ifndef __SRC_SMITH_" << tree_name_ << "_TASKS_H " << endl;
    tt << "#define __SRC_SMITH_" << tree_name_ << "_TASKS_H " << endl;
    tt << "" << endl;
    tt << "#include <memory>" << endl;
    tt << "#include <algorithm>" << endl;
    tt << "#include <src/smith/indexrange.h>" << endl;
    tt << "#include <src/smith/tensor.h>" << endl;
    tt << "#include <src/smith/task.h>" << endl;
    tt << "#include <src/smith/subtask.h>" << endl;
    tt << "#include <vector>" << endl;
    tt << "" << endl;
    tt << "namespace bagel {" << endl;
    tt << "namespace SMITH {" << endl;
    tt << "namespace " << tree_name_ << "{" << endl;
    tt << "" << endl;

    // virtual function, generate Task0 which zeros out the residual and starts zero level dependency queue.
    pair<string, string> rtmp = create_target(indent, icnt);
    ss << rtmp.first;
    tt << rtmp.second;
    ++icnt;

    // all the gamma tensors should be defined here. Only distinct Gammas are computed
    for (auto& i : gamma_) {
      i->set_num(icnt);
      assert(i->label().find("Gamma") != string::npos);
      ss << i->constructor_str(indent) << endl; 

      // switch for blas, if true merged rdm*f1 tensor multiplication will use blas 
      bool use_blas = false;
      tt << i->generate_gamma(icnt, use_blas);

#define debug_gamma
#ifdef debug_gamma
      cout << "\ndebug gamma" << endl;
      i->print();
      i->active()->print();
#endif

      vector<string> tmp = {i->label()};
      vector<int> rdms = i->active()->required_rdm(); 
      for (auto& j : rdms) {
        stringstream zz; 
        zz << "this->rdm" << j << "_";
        tmp.push_back(zz.str());
      }
      if (i->merged()) {
        stringstream mm;
        mm << "this->" << i->merged()->label() << "_";
        tmp.push_back(mm.str());
      }
      ss << generate_task(indent, 0, icnt, tmp);
      ++icnt;
    }

  } // end depth 0

  /////////////////////////////////////////////////////////////////
  // if op_ is not empty, we add a task that adds up op_.
  /////////////////////////////////////////////////////////////////
  if (!op_.empty()) {

    // step through operators and if they are new, construct them.
    if (find(ii.begin(), ii.end(), target_) == ii.end()) {
      ii.push_back(target_);
      ss << target_->constructor_str(indent) << endl;
    }

    vector<shared_ptr<Tensor>> op = {target_};
    op.insert(op.end(), op_.begin(), op_.end());
    ss << generate_task(indent, icnt, op, i0);

    list<shared_ptr<const Index>> ti = target_->index();

    // make sure no duplicates in tensor list for compute header & footer
    vector<shared_ptr<Tensor>> uniq_tensors;
    vector<string> tensor_labels;
    for (auto& i : op) {
      string label = label__(i->label());
      if (find(tensor_labels.begin(), tensor_labels.end(), label) != tensor_labels.end()) continue;
      tensor_labels.push_back(label);
      uniq_tensors.push_back(i);
    }

    tt << generate_compute_header(icnt, ti, uniq_tensors);
    tt << generate_compute_operators(indent, target_, op_);
    tt << generate_compute_footer(icnt, ti, uniq_tensors);

    ++icnt;
  }

  /////////////////////////////////////////////////////////////////
  // step through BinaryContraction
  /////////////////////////////////////////////////////////////////
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
    vector<shared_ptr<Tensor>> source_tensors = (*i)->tensors_vec();

    for (auto& s : source_tensors) {
      // if it contains a new intermediate tensor, dump a constructor
      // (while registering in ii - note that ii is a static variable here).
      if (find(ii.begin(), ii.end(), s) == ii.end() && s->label().find("I") != string::npos) {
        ii.push_back(s);
        ss << s->constructor_str(indent) << endl;
      }
    }
    // saving a counter to a protected member for dependency checks
    num_ = icnt;
    ss << generate_task(indent, num_, source_tensors, i0);


    // write out headers
    {
      list<shared_ptr<const Index>> ti = depth() != 0 ? (*i)->target_indices() : (*i)->ex_target_index();
      // if outer loop is empty, send inner loop indices to header
      if (ti.size() == 0) {
        assert(depth() != 0); 
        list<shared_ptr<const Index>> di = (*i)->loop_indices();
        tt << generate_compute_header(num_, di, source_tensors, true);
      } else { 
        tt << generate_compute_header(num_, ti, source_tensors);
      }
    }
    
    // use virtual function to generate a task for this binary contraction
    pair<string, string> btmp = generate_bc(indent, (*i));
    ss << btmp.first;
    tt << btmp.second;

    {
      // send outer loop indices if outer loop indices exist, otherwise send inner indices
      list<shared_ptr<const Index>> ti = depth() != 0 ? (*i)->target_indices() : (*i)->ex_target_index();
      if (depth() == 0)
        for (auto i = ti.begin(), j = ++ti.begin(); i != ti.end(); ++i, ++i, ++j, ++j)
          swap(*i, *j);
      if (ti.size() == 0) { 
        assert(depth() != 0);
        // sending inner indices
        list<shared_ptr<const Index>> di = (*i)->loop_indices();
        di.reverse();
        tt << generate_compute_footer(num_, di, source_tensors);
      } else {
        // sending outer indices
        tt << generate_compute_footer(num_, ti, source_tensors);
      }
   }

///////////////////////////////////////////////////////////////////////

    // increment icnt before going to subtrees
    ++icnt;
    // triggers a recursive call
    pair<string, string> tmp = (*i)->generate_task_list();
    ss << tmp.first;
    tt << tmp.second;
  } // end bc loop


  // go through additional tree graphs
  if (depth() == 0) {
    bool found_density = false; // temporary for testing  
    bool found_correction = false; // temporary for testing  
    for (auto& t : tree_list) {
        // energy queue here
        if (t->label()=="energy") {
          ss << "      std::shared_ptr<Queue<T>> energy_(new Queue<T>());" << endl;
          t->num_ = icnt;
          for (auto& j : t->bc_) {
            pair<string, string> tmp = j->generate_task_list(); 
            ss << tmp.first;
            tt << tmp.second;
          }
        } else if (t->label()=="correction") { 
          found_correction = true;
          ss << "      std::shared_ptr<Queue<T>> correction_(new Queue<T>());" << endl;
          t->num_ = icnt;
          for (auto& j : t->bc_) {
            pair<string, string> tmp = j->generate_task_list(); 
            ss << tmp.first;
            tt << tmp.second;
          }
          // end the energy tree and start density matrix generation
        } else if (t->label()=="density") { 
          found_density = true; 
          t->num_ = icnt;

          // save density task zero 
          i0 = icnt;

          // Create new task0 by using virtual function. Here the task0 target tensor is the density matrix.
          pair<string, string> dtmp = t->create_target(indent, icnt);
          ss << dtmp.first;
          tt << dtmp.second;
          ++icnt;

          // process the active space ie gamma..
          for (auto& g : t->gamma_) {
#ifdef debug_gamma
            cout << "\ndebug gamma dm" << endl;
            g->print();
            g->active()->print();
#endif
            assert(!g->merged());
            g->set_num(icnt);
            assert(g->label().find("Gamma") != string::npos);
            ss << g->constructor_str(indent) << endl; 

            // switch for blas, if true merged rdm*f1 tensor multiplication will use blas 
            bool use_blas = false;
            tt << g->generate_gamma(icnt, use_blas);
            vector<string> tmp = {g->label()};
            vector<int> rdms = g->active()->required_rdm(); 
            for (auto& j : rdms) {
              stringstream zz; 
              zz << "this->rdm" << j << "_";
              tmp.push_back(zz.str());
            }
            ss << t->generate_task(indent, i0, icnt, tmp, "", i0);
            ++icnt;
          }

          for (auto& j : t->bc_) {
            // if at top bc, add a task to for top level contraction for density matrix tensor 
            if (depth() == 0 ) { 

              vector<shared_ptr<Tensor>> source_tensors = j->tensors_vec();
              t->num_ = icnt;
              for (auto& s : source_tensors) {
                // if it contains a new intermediate tensor, dump a constructor
                // (while registering in ii - note that ii is a static variable here).
                if (find(ii.begin(), ii.end(), s) == ii.end() && s->label().find("I") != string::npos) {
                  ii.push_back(s);
                  ss << s->constructor_str(indent) << endl;
                }
              }
              ss << t->generate_task(indent, t->num_, source_tensors, i0);

              // these need to be reversed for this case
              list<shared_ptr<const Index>> proj = j->ex_target_index();
              proj.reverse();

              // write out headers
              {
                list<shared_ptr<const Index>> ti = depth() != 0 ? j->target_indices() : proj;
                // if outer loop is empty, send inner loop indices to header
                if (ti.size() == 0) {
                  assert(depth() != 0); 
                  list<shared_ptr<const Index>> di = j->loop_indices();
                  tt << t->generate_compute_header(t->num_, di, source_tensors, true);
                } else { 
                  tt << t->generate_compute_header(t->num_, ti, source_tensors);
                }
              }

              list<shared_ptr<const Index>> dm;
              assert(!(proj.size() & 1));
              for (auto ii = proj.begin(); ii != proj.end(); ++ii, ++ii) {
                auto k = ii; ++k;
                dm.push_back(*k);
                dm.push_back(*ii);
              }
              shared_ptr<Tensor> density(new Tensor(1.0, "d", dm));
              vector<shared_ptr<Tensor>> op2 = { j->next_target() };
              tt << generate_compute_operators(indent, density, op2, j->dagger());

              {
                // send outer loop indices if outer loop indices exist, otherwise send inner indices
                list<shared_ptr<const Index>> ti = depth() != 0 ? j->target_indices() : proj;
                if (depth() == 0)
                  for (auto m = ti.begin(), n = ++ti.begin(); m != ti.end(); ++m, ++m, ++n, ++n)
                    swap(*m, *n);
                if (ti.size() == 0) { 
                  assert(depth() != 0);
                  // sending inner indices
                  list<shared_ptr<const Index>> di = j->loop_indices();
                  di.reverse();
                  tt << t->generate_compute_footer(t->num_, di, source_tensors);
                } else {
                  // sending outer indices
                  tt << t->generate_compute_footer(t->num_, ti, source_tensors);
                }
             }
            // increment task counter
            ++icnt;
            }
       

            pair<string, string> tmp = j->generate_task_list();  
            ss << tmp.first;
            tt << tmp.second;
          }
          ++icnt;
        }
      
    } // end tree_list loop


    // generate computational algorithm
    if (!found_density) ss << "      std::shared_ptr<Queue<T>> density_(new Queue<T>());" << endl; 
    if (!found_correction) ss << "      std::shared_ptr<Queue<T>> correction_(new Queue<T>());" << endl; 

    ss << "      return make_tuple(queue_, energy_, density_, correction_);" << endl;
    ss << "    };" << endl;
    ss << endl;
    ss << "  public:" << endl;
    ss << "    " << tree_name_ << "(std::shared_ptr<const Reference> ref) : SpinFreeMethod<T>(ref), SMITH_info() {" << endl;
    ss << "      this->eig_ = this->f1_->diag();" << endl;
    ss << "      t2 = this->v2_->clone();" << endl;
    ss << "      e0_ = this->e0();" << endl;
    ss << "      this->update_amplitude(t2, this->v2_, true);" << endl;
    ss << "      t2->scale(2.0);" << endl;
    ss << "      r = t2->clone();" << endl;
    ss << "      d = this->h1_->clone();" << endl;
    ss << "    };" << endl;
    ss << "    ~" << tree_name_ << "() {}; " << endl;
    ss << "" << endl;
    ss << "    void solve() {" << endl;
    ss << "      this->print_iteration();" << endl;
    ss << "      int iter = 0;" << endl;
    ss << "      std::shared_ptr<Queue<T>> queue, energ, dens, correct;" << endl;
    ss << "      for ( ; iter != maxiter_; ++iter) {" << endl;
    ss << "        std::tie(queue, energ, dens, correct) = make_queue_();" << endl;
    ss << "        while (!queue->done())" << endl;
    ss << "          queue->next_compute();" << endl;
    ss << "        this->update_amplitude(t2, r);" << endl;
    ss << "        const double err = r->rms();" << endl;
    ss << "        r->zero();" << endl;
    ss << "        const double en = energy(energ);" << endl;
    ss << "        this->print_iteration(iter, en, err);" << endl;
    ss << "        if (err < thresh_residual()) break;" << endl;
    ss << "      }" << endl;
    ss << "      this->print_iteration(iter == maxiter_);" << endl;
    if (found_density) {
      ss << "      std::cout << \" === Unrelaxed density matrix ===\" << std::endl; " << endl;
      ss << "      while (!dens->done())" << endl;
      ss << "        dens->next_compute();" << endl;
      ss << "      d->scale(0.25);" << endl;
      ss << "      d->print2(\"density matrix\", 1.0e-5);" << endl;
    } if (found_correction) {
      ss << "      const double n = correction(correct);" << endl;
      ss << "      std::cout << \" correction: \" << std::setprecision(10) << n << std::endl;" << endl;
      ss << "      this->rdm1_->print2(\"rdm1\", 1.0e-5);" << endl;
    }
    ss << "    };" << endl;
    ss << "" << endl;
    ss << "    double energy(std::shared_ptr<Queue<T>> energ) {" << endl;
    ss << "      double en = 0.0;" << endl;
    ss << "      while (!energ->done()) {" << endl;
    ss << "        std::shared_ptr<Task<T>> c = energ->next_compute();" << endl;
    ss << "        en += c->energy() * 0.25;" << endl;  // 0.25 due to 1/2 each to bra and ket
    ss << "      }   " << endl;
    ss << "      return en; " << endl;
    ss << "    };  " << endl;
    ss << endl;
    // compute density matrix correction term needed for comparison to MP2 density  
    ss << "    double correction(std::shared_ptr<Queue<T>> correct) {" << endl;
    ss << "      double n = 0.0;" << endl;
    ss << "      while (!correct->done()) {" << endl;
    ss << "        std::shared_ptr<Task<T>> c = correct->next_compute();" << endl;
    ss << "        n += c->correction() * 0.25;" << endl;  // 0.25 due to 1/2 each to bra and ket
    ss << "      }   " << endl;
    ss << "      return n; " << endl;
    ss << "    };  " << endl;
    ss << endl;  // end comparison correction
    ss << "};" << endl;
    ss << endl;
    ss << "}" << endl;
    ss << "}" << endl;
    ss << "}" << endl;

    ss << "#endif" << endl << endl;

    tt << endl;
    tt << "}" << endl;
    tt << "}" << endl;
    tt << "}" << endl;
    tt << "#endif" << endl << endl; 
  }
  return make_pair(ss.str(), tt.str());
}


vector<shared_ptr<Tensor>> BinaryContraction::tensors_vec() {
  vector<shared_ptr<Tensor>> out;
  if (target_) out.push_back(target_);
  out.push_back(tensor_);
  if (!subtree_.empty())
    out.push_back(subtree_.front()->target());
  return out;
}


vector<int> BinaryContraction::required_rdm(vector<int> orig) const {
  vector<int> out = orig;
  for (auto& i : subtree_)
    out = i->required_rdm(out);

  sort(out.begin(), out.end());
  return out;
}

vector<int> Tree::required_rdm(vector<int> orig) const {
  vector<int> out = orig;
  for (auto& i : bc_)
    out = i->required_rdm(out);

  for (auto& i : op_) {
    if (i->label().find("Gamma") == string::npos) continue;
    vector<int> rdmn = i->active()->required_rdm();
    for (auto& j: rdmn)
      if (find(out.begin(), out.end(), j) == out.end()) out.push_back(j);
  }
  sort(out.begin(), out.end());
  return out;
}


