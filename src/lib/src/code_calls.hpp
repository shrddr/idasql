// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * code_calls.hpp - `disasm_calls` table (call sites within functions).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace code {

struct DisasmCallInfo {
  ea_t func_addr;
  ea_t ea;
  ea_t callee_addr;
  std::string callee_name;
};

GeneratorTableDef<DisasmCallInfo> define_disasm_calls();

} // namespace code
} // namespace idasql
