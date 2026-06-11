// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * code_cfg.hpp - `cfg_edges` table (intra-function control-flow edges) and the
 * disassembly convenience views.
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace code {

// cfg_edges table row
struct CfgEdgeInfo {
  ea_t func_ea;
  ea_t from_block;
  ea_t to_block;
  std::string edge_type; // "normal", "true", "false"
};

GeneratorTableDef<CfgEdgeInfo> define_cfg_edges();

// Registers disassembly convenience views (calls_in_loops, funcs_with_loops, ...)
bool register_disasm_views(xsql::Database &db);

} // namespace code
} // namespace idasql
