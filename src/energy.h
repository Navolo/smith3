//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: energy.h
// Copyright (C) 2013 Matthew MacLeod
//
// Author: Matthew MacLeod <matthew.macleod@northwestern.edu>
// Maintainer: Shiozaki group
//
// This file is part of the SMITH3 package.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


#ifndef __ENERGY_H
#define __ENERGY_H

#include "tree.h"

namespace smith {

/// Derived class for energy tree.
class Energy : public Tree {
  protected:


  public:
    /// Construct tree of equation pointers and set tree label.
    Energy(const std::shared_ptr<Equation> eq, std::string lab = "") : Tree(eq, lab) { }
    Energy(const std::shared_ptr<ListTensor> l, std::string lab, const bool t) : Tree(l, lab, t) { }
    virtual ~Energy() { }


    /// Return label of tree.
    std::string label() const override { return label_; }

    OutStream create_target(const int) const override { return OutStream(); }
    OutStream create_target_ci(const int) const override { return OutStream(); }
    std::shared_ptr<Tensor> create_tensor(std::list<std::shared_ptr<const Index>>) const override { return  std::shared_ptr<Tensor>(); }

    OutStream generate_task(const int ip, const int ic, const std::vector<std::string>, const std::string scalar = "", const int i0 = 0, bool der = false, bool diagonal = false) const override;
    OutStream generate_task_gamma(const int ip, const int ic, const std::vector<std::string>, const std::string scalar = "", const int i0 = 0, bool der = false, bool diagonal = false) const override { return OutStream(); }
    OutStream generate_compute_header(const int, const std::list<std::shared_ptr<const Index>> ti, const std::vector<std::shared_ptr<Tensor>>, const bool = false) const override;
    OutStream generate_compute_footer(const int, const std::list<std::shared_ptr<const Index>> ti, const std::vector<std::shared_ptr<Tensor>>, const bool dot) const override;
    OutStream generate_bc(std::shared_ptr<BinaryContraction>) const override;
    OutStream generate_bc_sources(const int, const std::list<std::shared_ptr<const Index>> ti, const std::vector<std::shared_ptr<Tensor>>, const bool, const bool, const std::shared_ptr<BinaryContraction>) const override { return OutStream(); }

};

}


#endif
