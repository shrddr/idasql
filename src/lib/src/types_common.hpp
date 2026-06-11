// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * types_common.hpp - Shared declarations for the idasql::types tables.
 *
 * Common includes plus the undo hook and type-kind classifier used across the
 * per-table type definition units (types_base.cpp, types_members.cpp, ...).
 * Each per-table type header includes this one.
 */

#pragma once

#include <idasql/platform.hpp>

#include <idasql/string_utils.hpp>
#include <idasql/vtable.hpp>
#include <xsql/database.hpp>
#include <xsql/vtable.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "dirtree_utils.hpp"
#include "ida_headers.hpp"

namespace idasql {
namespace types {

// Classify a tinfo_t into a coarse kind string (struct/union/enum/...)
const char *get_type_kind(const tinfo_t &tif);

} // namespace types
} // namespace idasql
