// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * code.hpp - Umbrella header for the idasql::code domain: the static-code view.
 *
 * Functions and structure (funcs, blocks, function_chunks, instructions,
 * instruction_operands + the operand-representation engine) plus the folded
 * disassembly analysis tables (disasm_calls, disasm_loops, call_graph,
 * shortest_path, cfg_edges).
 */

#pragma once

#include "code_funcs.hpp"
#include "code_blocks.hpp"
#include "code_operand_repr.hpp"
#include "code_instructions.hpp"
#include "code_calls.hpp"
#include "code_loops.hpp"
#include "code_graph.hpp"
#include "code_cfg.hpp"
