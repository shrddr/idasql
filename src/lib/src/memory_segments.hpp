// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_segments.hpp - `segments` table (start/end, name, class, perms).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace memory {

VTableDef define_segments();

} // namespace memory
} // namespace idasql
