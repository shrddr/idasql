// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_netnode_kv.hpp - `netnode_kv` table (netnode-backed key-value store).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace memory {

struct NetnodeKvRow {
  std::string key;
  std::string value;
};

CachedTableDef<NetnodeKvRow> define_netnode_kv();

} // namespace memory
} // namespace idasql
