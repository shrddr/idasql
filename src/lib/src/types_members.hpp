// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * types_members.hpp - `types_members` table (struct/union members), member
 * classification helpers, and the per-type member iterator.
 */

#pragma once

#include "types_common.hpp"

namespace idasql {
namespace types {

struct MemberEntry {
  uint32_t type_ordinal;
  std::string type_name;
  int member_index;
  std::string member_name;
  int64_t offset;
  int64_t offset_bits;
  int64_t size;
  int64_t size_bits;
  std::string member_type;
  bool is_bitfield;
  bool is_baseclass;
  std::string comment;
  // Member type classification (for efficient filtering)
  bool mt_is_struct;
  bool mt_is_union;
  bool mt_is_enum;
  bool mt_is_ptr;
  bool mt_is_array;
  int member_type_ordinal; // -1 if member type not in local types
};

int get_type_ordinal_by_name(til_t *ti, const char *type_name);

void classify_member_type(const tinfo_t &mtype, til_t *ti, bool &is_struct,
                          bool &is_union, bool &is_enum, bool &is_ptr,
                          bool &is_array, int &type_ordinal);

void collect_members(std::vector<MemberEntry> &rows);

struct TypeMemberRef {
  tinfo_t tif;
  udt_type_data_t udt;
  bool valid;
  uint32_t ordinal;

  TypeMemberRef(uint32_t ord);
  bool save();
};

bool build_member_entry(uint32_t ordinal, int member_index, MemberEntry &entry);

/**
 * Iterator for members of a specific type.
 * Used when query has: WHERE type_ordinal = X
 */
class MembersInTypeIterator : public xsql::RowIterator {
  uint32_t type_ordinal_;
  std::string type_name_;
  udt_type_data_t udt_;
  int idx_ = -1;
  bool valid_ = false;
  bool has_data_ = false;

public:
  explicit MembersInTypeIterator(uint32_t ordinal);
  bool next() override;
  bool eof() const override;
  void column(xsql::FunctionContext &ctx, int col) override;
  int64_t rowid() const override;
};

CachedTableDef<MemberEntry> define_types_members();

} // namespace types
} // namespace idasql
