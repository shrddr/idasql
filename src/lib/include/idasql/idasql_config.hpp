// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * idasql_config.hpp - Layout of the generic idasql per-database config netnode.
 *
 * idasql stores its own persistent, per-IDB configuration in a single reserved
 * netnode named "$ idasql config" (distinct from "$ idasql netnode_kv", which
 * is the user-facing key-value surface and must NOT be reused for tool config).
 *
 * This header defines ONLY the on-disk layout: the node name, a schema version,
 * and the reserved altval (integer) / supval (string) index constants. It is
 * intentionally free of any IDA SDK dependency so it can be included anywhere;
 * all netnode access lives in autostart_pin.cpp.
 *
 * Index allocation convention
 * ---------------------------
 *   altval and supval are independent index spaces. Indices 0x00..0x0F are
 *   reserved for node-global metadata; each feature gets its own 16-slot block
 *   starting at 0x10, 0x20, ... Add new features in a fresh block so older
 *   IDBs keep working and blocks never collide.
 */

#pragma once

#include <cstdint>

namespace idasql {
namespace config {

// Reserved netnode name for idasql's own per-IDB configuration.
inline constexpr const char *NODE_NAME = "$ idasql config";

// Bumped when the on-disk layout below changes in an incompatible way.
inline constexpr uint32_t SCHEMA_VERSION = 1;

// altval (integer) index map.
namespace alt {
// 0x00..0x0F: node-global metadata.
inline constexpr uint32_t SCHEMA_VERSION = 0x00;

// 0x10..0x1F: autostart feature block.
inline constexpr uint32_t AUTOSTART_BASE = 0x10;
inline constexpr uint32_t AUTOSTART_HTTP_PORT = AUTOSTART_BASE + 0; // 0x10
inline constexpr uint32_t AUTOSTART_MCP_PORT = AUTOSTART_BASE + 1;  // 0x11
inline constexpr uint32_t AUTOSTART_FLAGS = AUTOSTART_BASE + 2;     // 0x12
// 0x20.. reserved for the next feature.
} // namespace alt

// supval (string) index map (independent from altval).
namespace sup {
// 0x10..0x1F: autostart feature block.
inline constexpr uint32_t AUTOSTART_BASE = 0x10;
inline constexpr uint32_t AUTOSTART_HTTP_HOST = AUTOSTART_BASE + 0; // 0x10
inline constexpr uint32_t AUTOSTART_MCP_HOST = AUTOSTART_BASE + 1;  // 0x11
// 0x20.. reserved for the next feature.
} // namespace sup

// Bit positions stored in alt::AUTOSTART_FLAGS.
namespace autostart_flags {
inline constexpr uint64_t HTTP_ENABLED = 1ull << 0;
inline constexpr uint64_t MCP_ENABLED = 1ull << 1;
} // namespace autostart_flags

} // namespace config
} // namespace idasql
