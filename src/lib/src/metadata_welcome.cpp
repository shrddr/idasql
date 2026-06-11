// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <idasql/platform.hpp>

#include <sstream>
#include <string>
#include <vector>

#include "metadata_welcome.hpp"

#include "idasql_version.hpp"
#include "ida_headers.hpp"
#include <idasql/string_utils.hpp>

namespace idasql {
namespace metadata {
namespace {

using idasql::format_ea_hex;
using idasql::to_hex;

static std::string get_primary_entry_name() {
    if (get_entry_qty() <= 0) {
        return "";
    }
    qstring name;
    const uval_t ord = get_entry_ordinal(0);
    get_entry_name(&name, ord);
    return std::string(name.c_str());
}

static void collect_welcome(std::vector<WelcomeRow>& rows) {
    rows.clear();

    WelcomeRow row;
    row.idasql_version = IDASQL_VERSION_STRING;
    row.processor = inf_get_procname().c_str();
    row.is_64bit = inf_is_64bit() ? 1 : 0;
    row.min_ea = format_ea_hex(static_cast<uint64_t>(inf_get_min_ea()));
    row.max_ea = format_ea_hex(static_cast<uint64_t>(inf_get_max_ea()));
    row.start_ea = format_ea_hex(static_cast<uint64_t>(inf_get_start_ea()));

    row.entry_name = get_primary_entry_name();
    if (row.entry_name.empty()) {
        qstring fallback_name;
        if (get_name(&fallback_name, inf_get_start_ea()) > 0) {
            row.entry_name = fallback_name.c_str();
        }
    }

    row.funcs_count = static_cast<int>(get_func_qty());
    row.segments_count = static_cast<int>(get_segm_qty());
    row.names_count = static_cast<int>(get_nlist_size());
    row.strings_count = static_cast<int>(get_strlist_qty());

    // File-identity fields. Every API is bounded by the buffer size we pass and
    // guarded by its return code, falling back to an empty string when absent.
    char fname[QMAXPATH];
    row.filename = (get_root_filename(fname, sizeof(fname)) > 0) ? fname : "";

    char fpath[QMAXPATH];
    row.input_file_path = (get_input_file_path(fpath, sizeof(fpath)) > 0) ? fpath : "";

    // Full path of the IDB/I64 on disk (may differ from input_file_path if moved).
    const char* idb = get_path(PATH_TYPE_IDB);
    row.idb_path = (idb != nullptr) ? idb : "";

    uchar md5[16];
    row.md5 = retrieve_input_file_md5(md5) ? to_hex(md5, sizeof(md5)) : "";

    uchar sha256[32];
    row.sha256 = retrieve_input_file_sha256(sha256) ? to_hex(sha256, sizeof(sha256)) : "";

    std::ostringstream summary;
    summary << row.processor << " " << (row.is_64bit ? "64-bit" : "32-bit");
    if (!row.entry_name.empty()) {
        summary << " | entry: " << row.entry_name << " @ " << row.start_ea;
    } else {
        summary << " | start: " << row.start_ea;
    }
    summary << " | funcs: " << row.funcs_count;
    summary << " | segs: " << row.segments_count;
    summary << " | strings: " << row.strings_count;
    row.summary = summary.str();

    rows.push_back(std::move(row));
}

} // namespace

CachedTableDef<WelcomeRow> define_welcome() {
    return cached_table<WelcomeRow>("welcome")
        .no_shared_cache()
        .estimate_rows([]() -> size_t { return 1; })
        .cache_builder([](std::vector<WelcomeRow>& rows) {
            collect_welcome(rows);
        })
        .column_text("summary", [](const WelcomeRow& row) -> std::string { return row.summary; })
        .column_text("idasql_version", [](const WelcomeRow& row) -> std::string { return row.idasql_version; })
        .column_text("processor", [](const WelcomeRow& row) -> std::string { return row.processor; })
        .column_int("is_64bit", [](const WelcomeRow& row) -> int { return row.is_64bit; })
        .column_text("min_ea", [](const WelcomeRow& row) -> std::string { return row.min_ea; })
        .column_text("max_ea", [](const WelcomeRow& row) -> std::string { return row.max_ea; })
        .column_text("start_ea", [](const WelcomeRow& row) -> std::string { return row.start_ea; })
        .column_text("entry_name", [](const WelcomeRow& row) -> std::string { return row.entry_name; })
        .column_int("funcs_count", [](const WelcomeRow& row) -> int { return row.funcs_count; })
        .column_int("segments_count", [](const WelcomeRow& row) -> int { return row.segments_count; })
        .column_int("names_count", [](const WelcomeRow& row) -> int { return row.names_count; })
        .column_int("strings_count", [](const WelcomeRow& row) -> int { return row.strings_count; })
        .column_text("filename", [](const WelcomeRow& row) -> std::string { return row.filename; })
        .column_text("input_file_path", [](const WelcomeRow& row) -> std::string { return row.input_file_path; })
        .column_text("idb_path", [](const WelcomeRow& row) -> std::string { return row.idb_path; })
        .column_text("md5", [](const WelcomeRow& row) -> std::string { return row.md5; })
        .column_text("sha256", [](const WelcomeRow& row) -> std::string { return row.sha256; })
        .build();
}

} // namespace metadata
} // namespace idasql
