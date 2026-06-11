// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * idasql_commands.hpp - Dot-command parser for interactive sessions
 *
 * Shared command handling for CLI and plugin session frontends.
 */

#pragma once

#include <functional>
#include <string>

#include <xsql/thinclient/clipboard.hpp>
#include "welcome_query.hpp"

namespace idasql {

enum class CommandResult {
    NOT_HANDLED,  // Not a command, process as query
    HANDLED,      // Command executed successfully
    QUIT          // User requested quit
};

struct CommandCallbacks {
    std::function<std::string()> get_tables;
    std::function<std::string(const std::string&)> get_schema;
    std::function<std::string()> get_info;

    // MCP server callbacks (optional)
    std::function<std::string()> mcp_status;
    std::function<std::string(int, const std::string&)> mcp_start;
    std::function<std::string()> mcp_stop;

    // HTTP server callbacks (optional)
    std::function<std::string()> http_status;
    std::function<std::string(int, const std::string&)> http_start;
    std::function<std::string()> http_stop;

    // Autostart pin callbacks (optional). Wired by both the CLI and the plugin
    // (see common/pin_commands.hpp); the netnode work lives behind them so this
    // header has no IDA dependency. service is "http" | "mcp"; for pin_clear it
    // may also be "all".
    std::function<std::string()> pin_list;
    std::function<std::string(const std::string& service,
                              const std::string& bind, int port)> pin_set;
    std::function<std::string(const std::string& service, bool enable)> pin_enable;
    std::function<std::string(const std::string& service)> pin_clear;
};

inline void parse_bind_and_port(const std::string& raw, std::string& bind_addr, int& port) {
    bind_addr = "127.0.0.1";
    port = 0;

    std::string rest = raw;
    size_t rs = rest.find_first_not_of(" \t");
    if (rs == std::string::npos) {
        return;
    }
    rest = rest.substr(rs);

    std::string tok1;
    std::string tok2;
    size_t sp = rest.find_first_of(" \t");
    if (sp != std::string::npos) {
        tok1 = rest.substr(0, sp);
        size_t t2s = rest.find_first_not_of(" \t", sp);
        if (t2s != std::string::npos) {
            tok2 = rest.substr(t2s);
        }
    } else {
        tok1 = rest;
    }

    const bool tok1_numeric = !tok1.empty() && tok1.find_first_not_of("0123456789") == std::string::npos;
    if (tok1_numeric) {
        port = std::stoi(tok1);
    } else {
        bind_addr = tok1;
        if (!tok2.empty()) {
            port = std::stoi(tok2);
        }
    }
}

inline CommandResult handle_command(
    const std::string& input,
    const CommandCallbacks& callbacks,
    std::string& output) {
    if (input.empty() || input[0] != '.') {
        return CommandResult::NOT_HANDLED;
    }

    if (input == ".quit" || input == ".exit") {
        return CommandResult::QUIT;
    }

    if (input == ".tables") {
        if (callbacks.get_tables) {
            output = callbacks.get_tables();
        }
        return CommandResult::HANDLED;
    }

    if (input == ".info") {
        if (callbacks.get_info) {
            output = callbacks.get_info();
        }
        return CommandResult::HANDLED;
    }

    if (input == ".help") {
        output = "IDASQL Commands:\n"
                 "  .tables         List all tables\n"
                 "  .schema <table> Show table schema\n"
                 "  .info           Show database info\n"
                 "  .quit / .exit   Exit\n"
                 "  .help           Show this help\n"
#ifdef IDASQL_HAS_MCP
                 "\n"
                 "MCP Server:\n"
                 "  .mcp                     Show status or start if not running\n"
                 "  .mcp start [bind] [port] Start MCP server\n"
                 "  .mcp stop                Stop MCP server\n"
                 "  .mcp help                Show MCP help\n"
#endif
                 "\n"
                 "HTTP Server:\n"
                 "  .http                    Show status or start if not running\n"
                 "  .http start [bind] [port] Start HTTP server\n"
                 "  .http stop               Stop HTTP server\n"
                 "  .http help               Show HTTP help\n"
                 "\n"
                 "Autostart Pins:\n"
                 "  .pin                     Show pinned autostart config\n"
                 "  .pin set http|mcp [bind] <port>  Pin a server (plugin auto-starts on load)\n"
                 "  .pin on|off http|mcp     Enable/disable autostart-on-load\n"
                 "  .pin clear [http|mcp|all] Remove pinned config\n"
                 "  .pin help                Show pin help\n"
                 "\n"
                 "SQL:\n"
                 "  SELECT * FROM funcs LIMIT 10;\n"
                 "  SELECT name, size FROM funcs ORDER BY size DESC;\n";
        return CommandResult::HANDLED;
    }

    if (input.rfind(".mcp", 0) == 0) {
#ifdef IDASQL_HAS_MCP
        std::string subargs = input.length() > 4 ? input.substr(4) : "";
        size_t start = subargs.find_first_not_of(" \t");
        if (start != std::string::npos) {
            subargs = subargs.substr(start);
        }

        if (subargs.empty()) {
            if (callbacks.mcp_status) {
                output = callbacks.mcp_status();
            } else {
                output = "MCP server not available";
            }
        } else if (subargs.rfind("start", 0) == 0) {
            int port = 0;
            std::string bind_addr = "127.0.0.1";
            std::string rest = subargs.length() > 5 ? subargs.substr(5) : "";
            parse_bind_and_port(rest, bind_addr, port);

            if (callbacks.mcp_start) {
                output = callbacks.mcp_start(port, bind_addr);
                // Only copy to clipboard on fresh start (output reports a port).
                // Copy the MCP server JSON config (paste-ready into a client
                // such as Claude Desktop), not the human-readable status line.
                int started_port = 0;
                std::string started_host;
                if (xsql::thinclient::extract_mcp_start_endpoint(
                        output, started_host, started_port)) {
                    const std::string payload =
                        xsql::thinclient::build_mcp_clipboard_payload(
                            "idasql", started_host, started_port);
                    (void)xsql::thinclient::try_copy_text_to_clipboard_windows(payload);
                }
            } else {
                output = "MCP server not available";
            }
        } else if (subargs == "stop") {
            if (callbacks.mcp_stop) {
                output = callbacks.mcp_stop();
            } else {
                output = "MCP server not available";
            }
        } else if (subargs == "help") {
            output = "MCP Server Commands:\n"
                     "  .mcp                     Show status, start if not running\n"
                     "  .mcp start [bind] [port] Start MCP server (default: 127.0.0.1, random port)\n"
                     "  .mcp stop                Stop MCP server\n"
                     "  .mcp help                Show this help\n"
                     "\n"
                     "The MCP server exposes one tool:\n"
                     "  idasql_query  - Execute SQL query directly\n"
                     "\n"
                     "Connect with Claude Desktop by adding to config:\n"
                     "  {\"mcpServers\": {\"idasql\": {\"url\": \"http://127.0.0.1:<port>/sse\"}}}\n";
        } else {
            output = "Unknown MCP command: " + subargs + "\nUse '.mcp help' for available commands.";
        }
#else
        output = "MCP server support not compiled in. Rebuild with -DIDASQL_WITH_MCP=ON";
#endif
        return CommandResult::HANDLED;
    }

    if (input.rfind(".http", 0) == 0) {
        std::string subargs = input.length() > 5 ? input.substr(5) : "";
        size_t start = subargs.find_first_not_of(" \t");
        if (start != std::string::npos) {
            subargs = subargs.substr(start);
        }

        if (subargs.empty()) {
            if (callbacks.http_status) {
                output = callbacks.http_status();
            } else {
                output = "HTTP server not available";
            }
        } else if (subargs.rfind("start", 0) == 0) {
            int port = 0;
            std::string bind_addr = "127.0.0.1";
            std::string rest = subargs.length() > 5 ? subargs.substr(5) : "";
            parse_bind_and_port(rest, bind_addr, port);

            if (callbacks.http_start) {
                output = callbacks.http_start(port, bind_addr);
                // Only copy to clipboard on fresh start (multi-line output with URL)
                auto nl = output.find('\n');
                if (nl != std::string::npos) {
                    const std::string clipboard_text = output.substr(0, nl);
                    (void)xsql::thinclient::try_copy_text_to_clipboard_windows(clipboard_text);
                }
            } else {
                output = "HTTP server not available";
            }
        } else if (subargs == "stop") {
            if (callbacks.http_stop) {
                output = callbacks.http_stop();
            } else {
                output = "HTTP server not available";
            }
        } else if (subargs == "help") {
            const std::string example = idasql::format_query_curl_example("http://127.0.0.1:<port>");
            output = "HTTP Server Commands:\n"
                     "  .http                     Show status, start if not running\n"
                     "  .http start [bind] [port] Start HTTP server (default: 127.0.0.1, random port)\n"
                     "  .http stop                Stop HTTP server\n"
                     "  .http help                Show this help\n"
                     "\n"
                     "Endpoints:\n"
                     "  GET  /help       API documentation\n"
                     "  POST /query      Execute SQL (body = raw SQL)\n"
                     "  GET  /status     Health check\n"
                     "  POST /shutdown   Stop server\n"
                     "\n"
                     "Schema discovery:\n"
                     "  SELECT name, type FROM sqlite_master WHERE type IN ('table','view') ORDER BY type, name;\n"
                     "  PRAGMA table_info(funcs);\n"
                     "\n"
                     "Example:\n"
                     "  " + example + "\n";
        } else {
            output = "Unknown HTTP command: " + subargs + "\nUse '.http help' for available commands.";
        }
        return CommandResult::HANDLED;
    }

    if (input.rfind(".pin", 0) == 0) {
        static const char* kPinUnavailable =
            "Autostart pinning is not available in this session.";

        std::string subargs = input.length() > 4 ? input.substr(4) : "";
        size_t s = subargs.find_first_not_of(" \t");
        subargs = (s == std::string::npos) ? "" : subargs.substr(s);

        // Pop the next whitespace-delimited token off the front of rest.
        auto next_token = [](std::string& rest) -> std::string {
            size_t b = rest.find_first_not_of(" \t");
            if (b == std::string::npos) { rest.clear(); return ""; }
            rest = rest.substr(b);
            size_t e = rest.find_first_of(" \t");
            std::string tok = (e == std::string::npos) ? rest : rest.substr(0, e);
            rest = (e == std::string::npos) ? "" : rest.substr(e);
            return tok;
        };

        if (subargs.empty() || subargs == "list") {
            output = callbacks.pin_list ? callbacks.pin_list() : kPinUnavailable;
        } else if (subargs == "help") {
            output =
                "Autostart pin commands:\n"
                "  .pin                      Show pinned config (alias of .pin list)\n"
                "  .pin list                 Show pinned config\n"
#ifdef IDASQL_HAS_MCP
                "  .pin set http [bind] <port>  Pin HTTP host/port; enables autostart\n"
                "  .pin set mcp  [bind] <port>  Pin MCP host/port; enables autostart\n"
                "  .pin on  http|mcp         Enable autostart-on-load for a service\n"
                "  .pin off http|mcp         Disable autostart (keeps host/port)\n"
                "  .pin clear [http|mcp|all] Remove pinned config (default: all)\n"
#else
                "  .pin set http [bind] <port>  Pin HTTP host/port; enables autostart\n"
                "  .pin on  http             Enable autostart-on-load for HTTP\n"
                "  .pin off http             Disable autostart (keeps host/port)\n"
                "  .pin clear [http|all]     Remove pinned config (default: all)\n"
#endif
                "  .pin help                 Show this help\n"
                "\n"
#ifdef IDASQL_HAS_MCP
                "Pins are stored in the IDB. The plugin auto-starts enabled\n"
                "services when the database is opened; '.http start' / '.mcp start'\n"
                "with no explicit port reuse the pinned host/port. The port is\n"
                "required on '.pin set'; bind defaults to 127.0.0.1.\n";
#else
                "Pins are stored in the IDB. The plugin auto-starts the enabled\n"
                "service when the database is opened; '.http start' with no\n"
                "explicit port reuses the pinned host/port. The port is\n"
                "required on '.pin set'; bind defaults to 127.0.0.1.\n";
#endif
        } else {
            std::string verb = next_token(subargs);
            if (verb == "set") {
                std::string service = next_token(subargs);
                if (service != "http" && service != "mcp") {
                    output = "Usage: .pin set http|mcp [bind] <port>";
                } else {
                    std::string bind_addr;
                    int port = 0;
                    parse_bind_and_port(subargs, bind_addr, port);
                    if (port <= 0) {
                        output = "Error: .pin set requires an explicit port (e.g. '.pin set "
                                 + service + " 8080').";
                    } else if (callbacks.pin_set) {
                        output = callbacks.pin_set(service, bind_addr, port);
                    } else {
                        output = kPinUnavailable;
                    }
                }
            } else if (verb == "on" || verb == "off") {
                std::string service = next_token(subargs);
                if (service != "http" && service != "mcp") {
                    output = "Usage: .pin " + verb + " http|mcp";
                } else if (callbacks.pin_enable) {
                    output = callbacks.pin_enable(service, verb == "on");
                } else {
                    output = kPinUnavailable;
                }
            } else if (verb == "clear") {
                std::string service = next_token(subargs);
                if (service.empty()) service = "all";
                if (service != "http" && service != "mcp" && service != "all") {
                    output = "Usage: .pin clear [http|mcp|all]";
                } else if (callbacks.pin_clear) {
                    output = callbacks.pin_clear(service);
                } else {
                    output = kPinUnavailable;
                }
            } else {
                output = "Unknown pin command: " + verb
                         + "\nUse '.pin help' for available commands.";
            }
        }
        return CommandResult::HANDLED;
    }

    if (input.rfind(".schema", 0) == 0) {
        std::string table = input.length() > 8 ? input.substr(8) : "";
        size_t start = table.find_first_not_of(" \t");
        if (start != std::string::npos) {
            table = table.substr(start);
            size_t end = table.find_last_not_of(" \t");
            if (end != std::string::npos) {
                table = table.substr(0, end + 1);
            }
        } else {
            table.clear();
        }

        if (table.empty()) {
            output = "Usage: .schema <table_name>";
        } else if (callbacks.get_schema) {
            output = callbacks.get_schema(table);
        }
        return CommandResult::HANDLED;
    }

    output = "Unknown command: " + input;
    return CommandResult::HANDLED;
}

} // namespace idasql
