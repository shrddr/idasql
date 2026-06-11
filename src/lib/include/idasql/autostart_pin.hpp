// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * autostart_pin.hpp - Persisted "pin" preferences for autostarting servers.
 *
 * First consumer of the "$ idasql config" netnode (see idasql_config.hpp). A
 * pin records, per server kind (HTTP / MCP), a bind host, a port, and whether
 * the server should auto-start when the IDB is opened in the IDA plugin.
 *
 * Storage semantics:
 *   - port == 0            -> the service is not configured.
 *   - enabled (flag bit)   -> autostart-on-load (plugin only).
 *   host/port act as the remembered defaults for ".http start" / ".mcp start"
 *   with no explicit port; the enabled bit only governs autostart-on-load.
 *
 * This header is intentionally free of any IDA SDK dependency; the netnode
 * implementation lives in autostart_pin.cpp.
 */

#pragma once

#include <string>

namespace idasql {
namespace autostart {

enum class Service {
    Http,
    Mcp,
};

struct ServicePin {
    bool enabled = false;          // autostart-on-load
    int port = 0;                  // 0 == not configured
    std::string host = "127.0.0.1";
};

struct PinConfig {
    ServicePin http;
    ServicePin mcp;
};

// Read the autostart block from "$ idasql config". Returns defaults (port 0,
// disabled, host 127.0.0.1) when the node or a slot is absent.
PinConfig load();

// Store host+port for a service and enable its autostart bit. Creates the
// config node if needed and stamps the schema version.
void set(Service service, const std::string &host, int port);

// Toggle only the autostart-on-load bit (host/port preserved). Enabling
// requires the service to already have a configured port; otherwise no-op.
void set_enabled(Service service, bool enabled);

// Clear one service's port, host, and enabled bit (other config preserved).
void clear(Service service);

// Clear the entire autostart block (both services). The config node itself and
// any other (future) config blocks are preserved.
void clear_all();

} // namespace autostart
} // namespace idasql
