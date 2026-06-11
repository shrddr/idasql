// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * types_enums.hpp - `types_enum_values` table (enum constants) and the per-enum
 * value iterator.
 */

#pragma once

#include "types_common.hpp"

namespace idasql {
namespace types {

struct EnumValueEntry {
  uint32_t type_ordinal;
  std::string type_name;
  int value_index;
  std::string value_name;
  int64_t value;
  uint64_t uvalue;
  std::string comment;
};

void collect_enum_values(std::vector<EnumValueEntry> &rows);

struct EnumTypeRef {
  tinfo_t tif;
  enum_type_data_t ei;
  bool valid;
  uint32_t ordinal;

  EnumTypeRef(uint32_t ord);
  bool save();
};

bool build_enum_value_entry(uint32_t ordinal, int value_index,
                            EnumValueEntry &entry);

/**
 * Iterator for enum values of a specific enum type.
 * Used when query has: WHERE type_ordinal = X
 */
class EnumValuesInTypeIterator : public xsql::RowIterator {
  uint32_t type_ordinal_;
  std::string type_name_;
  enum_type_data_t ei_;
  int idx_ = -1;
  bool valid_ = false;
  bool has_data_ = false;

public:
  explicit EnumValuesInTypeIterator(uint32_t ordinal);
  bool next() override;
  bool eof() const override;
  void column(xsql::FunctionContext &ctx, int col) override;
  int64_t rowid() const override;
};

CachedTableDef<EnumValueEntry> define_types_enum_values();

} // namespace types
} // namespace idasql
