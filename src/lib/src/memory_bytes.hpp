// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_bytes.hpp - `bytes` generator table (raw/patched byte access).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace memory {

struct ByteRow {
  ea_t ea = BADADDR;
};

GeneratorTableDef<ByteRow> define_bytes();

} // namespace memory
} // namespace idasql
