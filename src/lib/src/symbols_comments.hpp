// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_comments.hpp - `comments` table (regular + repeatable comments).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace symbols {

struct CommentRow {
  ea_t ea = BADADDR;
  std::string comment;
  std::string rpt_comment;
};

void collect_comment_rows(std::vector<CommentRow> &rows);

CachedTableDef<CommentRow> define_comments();

} // namespace symbols
} // namespace idasql
