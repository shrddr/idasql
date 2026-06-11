// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * types_bookmarks.hpp - `local_type_bookmarks` table (bookmarks anchored at
 * local-type ordinals).
 */

#pragma once

#include "types_common.hpp"

namespace idasql {
namespace types {

struct LocalTypeBookmarkRow {
  uint32_t index = 0;
  uint32_t ordinal = 0;
  std::string type_name;
  std::string desc;
  uint64_t inode = 0;
  std::string folder_path;
  std::string full_path;
};

CachedTableDef<LocalTypeBookmarkRow> define_local_type_bookmarks();

} // namespace types
} // namespace idasql
