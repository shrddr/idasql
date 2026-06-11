// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "types_base.hpp"

namespace idasql {
namespace types {

void collect_types(std::vector<TypeEntry>& rows) {
    rows.clear();

    til_t* ti = get_idati();
    if (!ti) return;

    uint32_t max_ord = get_ordinal_limit(ti);
    if (max_ord == 0 || max_ord == uint32_t(-1)) return;

    auto folder_paths = dirtrees::collect_inode_paths(DIRTREE_LOCAL_TYPES);

    for (uint32_t ord = 1; ord < max_ord; ++ord) {
        const char* name = get_numbered_type_name(ti, ord);
        if (!name) continue;  // Skip gaps in ordinal space

        TypeEntry entry;
        entry.ordinal = ord;
        entry.name = name;
        auto path_it = folder_paths.find(static_cast<uint64_t>(ord));
        if (path_it != folder_paths.end()) {
            entry.folder_path = path_it->second.folder_path;
            entry.full_path = path_it->second.full_path;
        }

        tinfo_t tif;
        if (tif.get_numbered_type(ti, ord)) {
            entry.kind = get_type_kind(tif);
            entry.is_struct = tif.is_struct();
            entry.is_union = tif.is_union();
            entry.is_enum = tif.is_enum();
            entry.is_typedef = tif.is_typedef();
            entry.is_func = tif.is_func();
            entry.is_ptr = tif.is_ptr();
            entry.is_array = tif.is_array();

            // Get size
            size_t sz = tif.get_size();
            entry.size = (sz != BADSIZE) ? static_cast<int64_t>(sz) : -1;

            // Get alignment for structs/unions
            entry.alignment = 0;
            if (tif.is_struct() || tif.is_union()) {
                udt_type_data_t udt;
                if (tif.get_udt_details(&udt)) {
                    entry.alignment = static_cast<int>(udt.effalign);
                }
            }

            // Get definition string
            qstring def_str;
            tif.print(&def_str);
            entry.definition = def_str.c_str();

            // For typedefs, get the resolved type name
            if (tif.is_typedef()) {
                qstring res_name;
                if (tif.get_final_type_name(&res_name)) {
                    entry.resolved = res_name.c_str();
                }
            }
        } else {
            entry.kind = "unknown";
            entry.size = -1;
            entry.alignment = 0;
            entry.is_struct = false;
            entry.is_union = false;
            entry.is_enum = false;
            entry.is_typedef = false;
            entry.is_func = false;
            entry.is_ptr = false;
            entry.is_array = false;
        }

        rows.push_back(std::move(entry));
    }
}

// ============================================================================
// TYPES Table - All local types (enhanced)
// ============================================================================

CachedTableDef<TypeEntry> define_types() {
    return cached_table<TypeEntry>("types")
        .no_shared_cache()
        .estimate_rows([]() -> size_t {
            til_t* ti = get_idati();
            return ti ? static_cast<size_t>(get_ordinal_limit(ti)) : 0;
        })
        .cache_builder([](std::vector<TypeEntry>& rows) {
            collect_types(rows);
        })
        .column_int("ordinal", [](const TypeEntry& row) -> int {
            return static_cast<int>(row.ordinal);
        })
        .column_text_rw("name",
            [](const TypeEntry& row) -> std::string {
                return row.name;
            },
            [](TypeEntry& row, const char* new_name) -> bool {
                if (!new_name || !new_name[0]) {
                    xsql::set_vtab_error("types: name cannot be empty (ordinal=" + std::to_string(row.ordinal) + ")");
                    return false;
                }
                if (new_name == row.name) {
                    return true;
                }

                til_t* ti = get_idati();
                if (!ti) {
                    xsql::set_vtab_error("types: type library not available");
                    return false;
                }

                tinfo_t tif;
                if (!tif.get_numbered_type(ti, row.ordinal)) {
                    xsql::set_vtab_error("types: ordinal " + std::to_string(row.ordinal) + " not found");
                    return false;
                }

                bool ok = tif.rename_type(new_name) == TERR_OK;
                if (ok) row.name = new_name;
                else xsql::set_vtab_error("types: failed to rename ordinal " +
                                          std::to_string(row.ordinal) + " to '" + new_name + "'");
                return ok;
            })
        .column_text("kind", [](const TypeEntry& row) -> std::string {
            return row.kind;
        })
        .column_int64("size", [](const TypeEntry& row) -> int64_t {
            return row.size;
        })
        .column_int("alignment", [](const TypeEntry& row) -> int {
            return row.alignment;
        })
        .column_int("is_struct", [](const TypeEntry& row) -> int {
            return row.is_struct ? 1 : 0;
        })
        .column_int("is_union", [](const TypeEntry& row) -> int {
            return row.is_union ? 1 : 0;
        })
        .column_int("is_enum", [](const TypeEntry& row) -> int {
            return row.is_enum ? 1 : 0;
        })
        .column_int("is_typedef", [](const TypeEntry& row) -> int {
            return row.is_typedef ? 1 : 0;
        })
        .column_int("is_func", [](const TypeEntry& row) -> int {
            return row.is_func ? 1 : 0;
        })
        .column_int("is_ptr", [](const TypeEntry& row) -> int {
            return row.is_ptr ? 1 : 0;
        })
        .column_int("is_array", [](const TypeEntry& row) -> int {
            return row.is_array ? 1 : 0;
        })
        .column_text("definition", [](const TypeEntry& row) -> std::string {
            return row.definition;
        })
        .column_text("resolved", [](const TypeEntry& row) -> std::string {
            return row.resolved;
        })
        .column_text_nullable_rw("folder_path",
            [](const TypeEntry& row) -> std::optional<std::string> {
                if (row.folder_path.empty()) {
                    return std::nullopt;
                }
                return row.folder_path;
            },
            [](TypeEntry& row, xsql::FunctionArg val) -> bool {
                const bool ok = dirtrees::move_inode_to_folder(
                    DIRTREE_LOCAL_TYPES, static_cast<uint64_t>(row.ordinal),
                    row.name, val, "types.folder_path");
                if (ok) {
                    auto path = dirtrees::find_inode_path(
                        DIRTREE_LOCAL_TYPES, static_cast<uint64_t>(row.ordinal));
                    if (path) {
                        row.folder_path = path->folder_path;
                        row.full_path = path->full_path;
                    }
                }
                return ok;
            })
        .column_text("full_path", [](const TypeEntry& row) -> std::string {
            return row.full_path;
        })
        .deletable([](TypeEntry& row) -> bool {
            til_t* ti = get_idati();
            if (!ti) return false;
            return del_numbered_type(ti, row.ordinal);
        })
        .insertable([](int argc, xsql::FunctionArg* argv) -> bool {
            if (argc < 2 || argv[1].is_null())
                return false;

            const char* name = argv[1].as_c_str();
            if (!name || !name[0]) return false;

            // kind (col 2): defaults to "struct"
            std::string kind = "struct";
            if (argc > 2 && !argv[2].is_null()) {
                const char* k = argv[2].as_c_str();
                if (k && k[0]) kind = k;
            }

            til_t* ti = get_idati();
            if (!ti) return false;

            // Check if type with this name already exists
            if (get_type_ordinal(ti, name) != 0)
                return false;

            uint32_t ord = alloc_type_ordinal(ti);
            if (ord == 0) return false;

            tinfo_t tif;
            if (kind == "struct") {
                udt_type_data_t udt;
                udt.is_union = false;
                tif.create_udt(udt);
            } else if (kind == "union") {
                udt_type_data_t udt;
                udt.is_union = true;
                tif.create_udt(udt);
            } else if (kind == "enum") {
                enum_type_data_t ei;
                tif.create_enum(ei);
            } else {
                return false;
            }

            return tif.set_numbered_type(ti, ord, NTF_REPLACE, name) == TERR_OK;
        })
        .build();
}

// ============================================================================
// APPLIED_TYPES Table - Type bindings at addresses
// ============================================================================

} // namespace types
} // namespace idasql
