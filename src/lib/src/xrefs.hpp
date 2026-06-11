// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_xrefs.hpp - `xrefs` and `data_refs` tables, plus constraint-pushdown
 * iterators (xrefs to/from an address, xrefs from a function).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace xrefs {

struct XrefInfo {
  ea_t from_ea;
  ea_t to_ea;
  ea_t from_func = BADADDR; // Pre-computed: function containing from_ea
  uint8_t type;
  bool is_code;
};

struct DataRefInfo {
  ea_t from_ea = BADADDR;
  ea_t to_ea = BADADDR;
  ea_t from_func = BADADDR;
  uint8_t type = 0;
};

/**
 * Iterator for xrefs TO a specific address.
 * Used when query has: WHERE to_ea = X
 * Uses xrefblk_t::first_to/next_to for O(refs_to_X) instead of O(all_xrefs)
 */
class XrefsToIterator : public xsql::RowIterator {
  ea_t target_;
  xrefblk_t xb_;
  bool started_ = false;
  bool valid_ = false;

public:
  explicit XrefsToIterator(ea_t target);
  bool next() override;
  bool eof() const override;
  void column(xsql::FunctionContext &ctx, int col) override;
  int64_t rowid() const override;
};

/**
 * Iterator for xrefs FROM a specific address.
 * Used when query has: WHERE from_ea = X
 * Uses xrefblk_t::first_from/next_from for O(refs_from_X) instead of
 * O(all_xrefs)
 */
class XrefsFromIterator : public xsql::RowIterator {
  ea_t source_;
  xrefblk_t xb_;
  bool started_ = false;
  bool valid_ = false;

public:
  explicit XrefsFromIterator(ea_t source);
  bool next() override;
  bool eof() const override;
  void column(xsql::FunctionContext &ctx, int col) override;
  int64_t rowid() const override;
};

/**
 * Iterator for xrefs originating from within a specific function.
 * Used when query has: WHERE from_func = X
 * Iterates all code items in the function and enumerates their outgoing xrefs.
 */
class XrefsFromFuncIterator : public xsql::RowIterator {
  ea_t func_ea_;
  func_item_iterator_t fii_;
  xrefblk_t xb_;
  bool fii_valid_ = false;
  bool xb_valid_ = false;
  bool started_ = false;
  bool eof_ = false;

  bool advance_to_next_xref();

public:
  explicit XrefsFromFuncIterator(ea_t func_ea);
  bool next() override;
  bool eof() const override;
  void column(xsql::FunctionContext &ctx, int col) override;
  int64_t rowid() const override;
};

CachedTableDef<XrefInfo> define_xrefs();
CachedTableDef<DataRefInfo> define_data_refs();

} // namespace xrefs
} // namespace idasql
