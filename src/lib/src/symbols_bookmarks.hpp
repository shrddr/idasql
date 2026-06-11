// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_bookmarks.hpp - `bookmarks` table (marked locations + descriptions).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace symbols {

struct BookmarkRow {
  uint32_t index = 0;
  ea_t ea = BADADDR;
  std::string desc;
  uint64_t inode = 0;
  std::string folder_path;
  std::string full_path;
};

void collect_bookmark_rows(std::vector<BookmarkRow> &rows);

CachedTableDef<BookmarkRow> define_bookmarks();

} // namespace symbols
} // namespace idasql
