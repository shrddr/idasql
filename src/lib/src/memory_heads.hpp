// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_heads.hpp - `heads` generator table (defined items) + helpers.
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace memory {

struct HeadRow {
  ea_t ea = BADADDR;
};

void collect_head_rows(std::vector<HeadRow> &rows);
const char *get_item_type_str(ea_t ea);

GeneratorTableDef<HeadRow> define_heads();

} // namespace memory
} // namespace idasql
