// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_names.hpp - `names` and `entries` tables (named addresses, entry
 * points).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace symbols {

struct NameRow {
  ea_t ea = BADADDR;
  std::string name;
  int is_public = 0;
  int is_weak = 0;
  std::string folder_path;
  std::string full_path;
};

void collect_name_rows(std::vector<NameRow> &rows);

CachedTableDef<NameRow> define_names();
VTableDef define_entries();

} // namespace symbols
} // namespace idasql
