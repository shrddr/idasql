// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "types_common.hpp"

namespace idasql {
namespace types {

// ============================================================================
// Type Kind Classification
// ============================================================================

const char* get_type_kind(const tinfo_t& tif) {
    if (tif.is_struct()) return "struct";
    if (tif.is_union()) return "union";
    if (tif.is_enum()) return "enum";
    if (tif.is_typedef()) return "typedef";
    if (tif.is_func()) return "func";
    if (tif.is_ptr()) return "ptr";
    if (tif.is_array()) return "array";
    return "other";
}

// ============================================================================
// Type Entry Cache
// ============================================================================

} // namespace types
} // namespace idasql
