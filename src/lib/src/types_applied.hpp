// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * types_applied.hpp - `applied_types` table (type bindings at addresses).
 */

#pragma once

#include "types_common.hpp"

namespace idasql {
namespace types {

struct AppliedTypeEntry {
  ea_t ea = BADADDR;
};

GeneratorTableDef<AppliedTypeEntry> define_applied_types();

} // namespace types
} // namespace idasql
