// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "code_funcs.hpp"

#include "decompiler.hpp"

using namespace idasql::core;

namespace idasql {
namespace code {

bool update_function_comment(FuncRow &row, xsql::FunctionArg val,
                             bool repeatable) {
  if (val.is_nochange()) {
    return true;
  }

  func_t *f = get_func(row.start_ea);
  if (!f)
    return false;

  const char *new_comment = val.is_null() ? nullptr : val.as_c_str();
  const std::string requested_comment = new_comment ? new_comment : "";
  std::string &original_comment =
      repeatable ? row.original_rpt_comment : row.original_comment;

  // no_shared_cache() can replay the pre-update value during unrelated
  // column updates; treat that exact replay as a no-op.
  if (requested_comment == original_comment) {
    return true;
  }

  idasql_auto_wait();
  const bool ok = set_func_cmt(f, requested_comment.c_str(), repeatable);
  if (ok) {
    original_comment = requested_comment;
    decompiler::invalidate_decompiler_cache(row.start_ea);
  }
  idasql_auto_wait();
  return ok;
}

// ============================================================================
// FUNCS Table (with UPDATE/DELETE support)
// ============================================================================

CachedTableDef<FuncRow> define_funcs() {
  return cached_table<FuncRow>("funcs")
      .no_shared_cache()
      .estimate_rows([]() -> size_t { return get_func_qty(); })
      .count([]() -> size_t { return get_func_qty(); })
      .cache_builder([](std::vector<FuncRow> &rows) {
        rows.clear();
        const size_t n = get_func_qty();
        rows.reserve(n);
        auto folder_paths = dirtrees::collect_inode_paths(DIRTREE_FUNCS);
        for (size_t i = 0; i < n; ++i) {
          func_t *f = getn_func(i);
          if (f) {
            FuncRow row;
            row.start_ea = f->start_ea;
            auto it = folder_paths.find(static_cast<uint64_t>(f->start_ea));
            if (it != folder_paths.end()) {
              row.folder_path = it->second.folder_path;
              row.full_path = it->second.full_path;
            }
            rows.push_back(std::move(row));
          }
        }
      })
      .row_lookup([](FuncRow &row, int64_t rowid) -> bool {
        func_t *f = getn_func(static_cast<size_t>(rowid));
        if (!f)
          return false;
        row.start_ea = f->start_ea;
        row.original_name = safe_func_name(row.start_ea);
        row.original_prototype = safe_func_prototype(row.start_ea);
        row.original_comment = safe_func_comment(row.start_ea, false);
        row.original_rpt_comment = safe_func_comment(row.start_ea, true);
        auto path = dirtrees::find_inode_path(DIRTREE_FUNCS,
                                              static_cast<uint64_t>(row.start_ea));
        if (path) {
          row.folder_path = path->folder_path;
          row.full_path = path->full_path;
        }
        return true;
      })
      .column_int64("address",
                    [](const FuncRow &row) -> int64_t {
                      return static_cast<int64_t>(row.start_ea);
                    })
      .column_text_rw(
          "name",
          [](const FuncRow &row) -> std::string {
            return safe_func_name(row.start_ea);
          },
          [](FuncRow &row, const char *new_name) -> bool {
            const std::string requested_name = new_name ? new_name : "";
            if (requested_name == row.original_name) {
              return true;
            }
            idasql_auto_wait();
            bool ok =
                set_name(row.start_ea, requested_name.c_str(), SN_CHECK) != 0;
            if (ok)
              decompiler::invalidate_decompiler_cache(row.start_ea);
            idasql_auto_wait();
            return ok;
          })
      .column_text_rw(
          "prototype",
          [](const FuncRow &row) -> std::string {
            return safe_func_prototype(row.start_ea);
          },
          [](FuncRow &row, xsql::FunctionArg val) -> bool {
            if (val.is_nochange()) {
              return true;
            }
            const char *new_decl = val.is_null() ? nullptr : val.as_c_str();
            const std::string requested_decl = new_decl ? new_decl : "";
            // no_shared_cache() can replay the pre-update declaration during
            // rename-only updates; treat that exact replay as a no-op.
            if (requested_decl == row.original_prototype) {
              return true;
            }
            idasql_auto_wait();
            bool ok = false;
            if (new_decl == nullptr || new_decl[0] == '\0') {
              del_tinfo(row.start_ea);
              ok = true;
            } else {
              ok = apply_cdecl(nullptr, row.start_ea, new_decl, 0);
            }
            if (ok)
              decompiler::invalidate_decompiler_cache(row.start_ea);
            idasql_auto_wait();
            return ok;
          })
      .column_text_rw(
          "comment",
          [](const FuncRow &row) -> std::string {
            return safe_func_comment(row.start_ea, false);
          },
          [](FuncRow &row, xsql::FunctionArg val) -> bool {
            return update_function_comment(row, val, false);
          })
      .column_text_rw(
          "rpt_comment",
          [](const FuncRow &row) -> std::string {
            return safe_func_comment(row.start_ea, true);
          },
          [](FuncRow &row, xsql::FunctionArg val) -> bool {
            return update_function_comment(row, val, true);
          })
      .column_int64("size",
                    [](const FuncRow &row) -> int64_t {
                      func_t *f = get_func(row.start_ea);
                      return f ? static_cast<int64_t>(f->size()) : 0;
                    })
      .column_int64("end_ea",
                    [](const FuncRow &row) -> int64_t {
                      func_t *f = get_func(row.start_ea);
                      return f ? static_cast<int64_t>(f->end_ea) : 0;
                    })
      .column_int64_rw(
          "flags",
          [](const FuncRow &row) -> int64_t {
            func_t *f = get_func(row.start_ea);
            return f ? static_cast<int64_t>(f->flags) : 0;
          },
          [](FuncRow &row, int64_t new_flags) -> bool {
            func_t *f = get_func(row.start_ea);
            if (!f)
              return false;
            f->flags = static_cast<ushort>(new_flags);
            bool ok = update_func(f);
            if (ok)
              decompiler::invalidate_decompiler_cache(row.start_ea);
            return ok;
          })
      // Prototype columns - return type (lazy-computed, cached per row)
      .column_text("return_type",
                   [](const FuncRow &row) -> std::string {
                     if (!row.ensure_fi())
                       return "";
                     qstring ret_str;
                     row.fi.rettype.print(&ret_str);
                     return ret_str.c_str();
                   })
      .column_int("return_is_ptr",
                  [](const FuncRow &row) -> int {
                    return row.ensure_fi() && row.fi.rettype.is_ptr() ? 1 : 0;
                  })
      .column_int("return_is_int",
                  [](const FuncRow &row) -> int {
                    return row.ensure_fi() && row.fi.rettype.is_int() ? 1 : 0;
                  })
      .column_int("return_is_integral",
                  [](const FuncRow &row) -> int {
                    return row.ensure_fi() && row.fi.rettype.is_integral() ? 1
                                                                           : 0;
                  })
      .column_int("return_is_void",
                  [](const FuncRow &row) -> int {
                    return row.ensure_fi() && row.fi.rettype.is_void() ? 1 : 0;
                  })
      // Prototype columns - arguments
      .column_int("arg_count",
                  [](const FuncRow &row) -> int {
                    if (!row.ensure_fi())
                      return 0;
                    return static_cast<int>(row.fi.size());
                  })
      .column_text("calling_conv",
                   [](const FuncRow &row) -> std::string {
                     if (!row.ensure_fi())
                       return "";
                     return get_cc_name(row.fi.get_cc());
                   })
      .column_text_nullable_rw(
          "folder_path",
          [](const FuncRow &row) -> std::optional<std::string> {
            if (row.folder_path.empty())
              return std::nullopt;
            return row.folder_path;
          },
          [](FuncRow &row, xsql::FunctionArg val) -> bool {
            const bool ok = dirtrees::move_inode_to_folder(
                DIRTREE_FUNCS, static_cast<uint64_t>(row.start_ea),
                safe_func_name(row.start_ea), val, "funcs.folder_path");
            if (ok) {
              auto path = dirtrees::find_inode_path(
                  DIRTREE_FUNCS, static_cast<uint64_t>(row.start_ea));
              if (path) {
                row.folder_path = path->folder_path;
                row.full_path = path->full_path;
              }
              decompiler::invalidate_decompiler_cache(row.start_ea);
            }
            return ok;
          })
      .column_text("full_path", [](const FuncRow &row) -> std::string {
        return row.full_path;
      })
      .deletable([](FuncRow &row) -> bool {
        idasql_auto_wait();
        bool ok = del_func(row.start_ea);
        idasql_auto_wait();
        return ok;
      })
      .insertable([](int argc, xsql::FunctionArg *argv) -> bool {
        // address (col 0) is required
        if (argc < 1 || argv[0].is_null())
          return false;

        ea_t ea = static_cast<ea_t>(argv[0].as_int64());

        // Check if function already exists at this address
        if (get_func(ea) != nullptr)
          return false;

        idasql_auto_wait();
        // end_ea from col 4 if provided, else BADADDR (IDA auto-detects)
        ea_t end = BADADDR;
        if (argc > 4 && !argv[4].is_null())
          end = static_cast<ea_t>(argv[4].as_int64());

        bool ok = add_func(ea, end);
        idasql_auto_wait();

        if (!ok)
          return false;

        // Optional: set name (col 1) after creation
        if (argc > 1 && !argv[1].is_null()) {
          const char *name = argv[1].as_c_str();
          if (name && name[0])
            set_name(ea, name, SN_CHECK);
        }

        return true;
      })
      .build();
}

} // namespace code
} // namespace idasql
