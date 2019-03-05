//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: rdmI0.h
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

#ifndef __SRC_RDMI0_H
#define __SRC_RDMI0_H

#include <map>
#include "index.h"

namespace smith {

/// A derived class for modified reduced density matrices which have partial reference wave function bras and kets (<I|E|0>). Used to generate RDM summation tasks for Method_tasks.h file.
class RDMI0 : public RDM {
  protected:

    /// Generate get block - source data to be added to target (move block).
    std::string make_get_block(std::string indent, std::string tag, std::string lbl, const std::list<std::shared_ptr<const Index>>& index);
    /// Generate get block - source data to be added to target (move block).
    std::string make_get_out_block(std::string indent, std::string tag, std::string lbl, const std::list<std::shared_ptr<const Index>>& index);
    /// Generate get block - source data to be added to target (move block).
    std::string make_out_block(std::string indent, std::string tag, std::string lbl, const std::list<std::shared_ptr<const Index>>& index);
    /// Generate sort_indices which makes array. This version has no addition (or factor multiplication-0111).
    std::string make_sort_indices(std::string indent, std::string tag, const std::list<std::shared_ptr<const Index>>& loop, const std::list<std::shared_ptr<const Index>>& index);
    /// Generates RDM and merged (fock) tensor multipication.
    std::string multiply_merge_sources(const std::string itag, std::string& indent,  const std::list<std::shared_ptr<const Index>>& merged, const std::list<std::shared_ptr<const Index>>& index);
    /// Generates RDM and merged (fock) tensor multipication.
    std::string multiply_merge(const std::string itag, std::string& indent,  const std::list<std::shared_ptr<const Index>>& merged, const std::list<std::shared_ptr<const Index>>& index);
    /// If delta case, also makes index loops then checks to see if merged-or-delta indices are in loops..
    std::string make_merged_loops(std::string& indent, const std::string tag, std::vector<std::string>& close, const std::list<std::shared_ptr<const Index>>& index, const bool overwrite = false);
    /// Adds merged (fock) tensor with indices, used by muliply_merge member, as well as ci index tensor multiplication in case of rdm0.
    std::string fdata_mult(const std::string itag, const std::list<std::shared_ptr<const Index>>& merged, const std::list<std::shared_ptr<const Index>>& ci_index, const bool end = true);

    // virtual
    /// Generate entire task code for Gamma RDM summation.
    std::string generate_not_merged(std::string indent, const std::string tlab, const std::list<std::shared_ptr<const Index>>& loop, std::vector<std::string> in_tensors) override;
    /// Generates entire task code for Gamma RDM summation with merged object (additional tensor, here fock tensor) multiplication.
    std::string generate_merged(std::string indent, const std::string itag, const std::list<std::shared_ptr<const Index>>& index, const std::list<std::shared_ptr<const Index>>& merged, const std::string mlab, std::vector<std::string> in_tensors, const bool use_blas) override;

    // for gamma - merged
    /// Generate entire task code for Gamma RDM summation.
    std::string generate_not_merged_sources(std::string indent, const std::string tlab, const std::list<std::shared_ptr<const Index>>& loop, std::vector<std::string> in_tensors);
    /// Generates entire task code for Gamma RDM summation with merged object (additional tensor, here fock tensor) multiplication.
    std::string generate_merged_sources(std::string indent, const std::string itag, const std::list<std::shared_ptr<const Index>>& index, const std::list<std::shared_ptr<const Index>>& merged, const std::string mlab, std::vector<std::string> in_tensors, const bool use_blas);

    /// Makes if statement in delta cases ie index equivalency check line.
    std::string make_delta_if(std::string& indent, std::vector<std::string>& close) override;
    /// Replaces tensor labels to more general labels in(x), where x is a counter for in tensors. RDM tensors numbered before merged (fock) tensor. Eg, rdm1 is mapped to in(0), rdm2 -> in(1), and in merged case with max rdm2, f1 -> in(2).
    void map_in_tensors(std::vector<std::string> in_tensors, std::map<std::string,std::string>& inlab) override;

    /// Loops over delta indices in Gamma summation.
    std::string make_sort_loops(const std::string itag, std::string& indent, const std::list<std::shared_ptr<const Index>>& index, std::vector<std::string>& close) override;

    // for task summation line
    /// Generates odata (Gamma) part of for summation ie LHS in equations gamma += rdm or gamma += rdm * f1
    std::string make_odata_sources(const std::string itag, std::string& indent, const std::list<std::shared_ptr<const Index>>& index);
    std::string make_odata(const std::string itag, std::string& indent, const std::list<std::shared_ptr<const Index>>& index) override;

    /// Do blas multiplication of Gamma and fock tensors...not implemented yet for subtask code!
    std::string make_blas_multiply(std::string indent, const std::list<std::shared_ptr<const Index>>& loop, const std::list<std::shared_ptr<const Index>>& index) override;
    /// Used for blas multiplication of RDM and merged (fock) tensors. NB not implemented yet for subtask code!
    std::pair<std::string, std::string> get_dim(const std::list<std::shared_ptr<const Index>>& di, const std::list<std::shared_ptr<const Index>>& index) const override;


  public:
    /// Make RDM object from list of indices, delta indices and factor.
    RDMI0(const std::list<std::shared_ptr<const Index>>& in,
        const std::map<std::shared_ptr<const Index>, std::shared_ptr<const Index>>& in2, std::pair<bool, bool> braket,
        const double& f = 1.0)
      : RDM(in, in2, braket, f) { }
    /// Copy RDM but use new indices for index. Useful when have kets, see active reduce.
    RDMI0(RDM& o, std::list<std::shared_ptr<const Index>>& in)
      : RDM(in, o.delta(), o.braket(), o.fac()) { }
    virtual ~RDMI0() { }

    /// Reverse order and dagger for list of indices.
    std::list<std::shared_ptr<const Index>> conjugate() override;

    /// Copies this rdm.
    std::shared_ptr<RDM> copy() const override;

    /// Application of Wick's theorem and is controlled by const Index::num_. See active.cc. One index is going to be annihilated. done is updated inside the function.
    std::list<std::shared_ptr<RDM>> reduce_one(std::list<int>& done) const override;

    /// Generate Gamma summation task, for both non-merged and merged case (RDM * f1 tensor multiplication).
    std::string generate(std::string indent, const std::string itag, const std::list<std::shared_ptr<const Index>>& index, const std::list<std::shared_ptr<const Index>>& merged, const std::string mlab, std::vector<std::string> in_tensors, const bool use_blas) override;

    /// Generate Gamma summation task, for both non-merged and merged case (RDM * f1 tensor multiplication).
    std::string generate_sources(std::string indent, const std::string itag, const std::list<std::shared_ptr<const Index>>& index, const std::list<std::shared_ptr<const Index>>& merged, const std::string mlab, std::vector<std::string> in_tensors, const bool use_blas) override;


};

}

#endif
