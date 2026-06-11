// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "symbols_comments.hpp"

using namespace idasql::core;

namespace idasql {
namespace symbols {

// ============================================================================
// COMMENTS Table (with UPDATE/DELETE support)
// ============================================================================

void collect_comment_rows(std::vector<CommentRow> &rows) {
  rows.clear();

  ea_t ea = inf_get_min_ea();
  ea_t max_ea = inf_get_max_ea();

  while (ea < max_ea) {
    qstring cmt;
    qstring rpt;
    bool has_cmt = get_cmt(&cmt, ea, false) > 0;
    bool has_rpt = get_cmt(&rpt, ea, true) > 0;

    if (has_cmt || has_rpt) {
      CommentRow row;
      row.ea = ea;
      row.comment = has_cmt ? std::string(cmt.c_str()) : std::string();
      row.rpt_comment = has_rpt ? std::string(rpt.c_str()) : std::string();
      rows.push_back(std::move(row));
    }

    ea = next_head(ea, max_ea);
    if (ea == BADADDR)
      break;
  }
}

CachedTableDef<CommentRow> define_comments() {
  return cached_table<CommentRow>("comments")
      .no_shared_cache()
      .estimate_rows(
          []() -> size_t { return static_cast<size_t>(get_nlist_size()); })
      .cache_builder(
          [](std::vector<CommentRow> &rows) { collect_comment_rows(rows); })
      .row_populator([](CommentRow &row, int argc, xsql::FunctionArg *argv) {
        // argv[2]=address, argv[3]=comment, argv[4]=rpt_comment
        if (argc > 2)
          row.ea = static_cast<ea_t>(argv[2].as_int64());
        if (argc > 3 && !argv[3].is_null()) {
          const char *c = argv[3].as_c_str();
          row.comment = c ? c : "";
        }
        if (argc > 4 && !argv[4].is_null()) {
          const char *c = argv[4].as_c_str();
          row.rpt_comment = c ? c : "";
        }
      })
      .column_int64("address",
                    [](const CommentRow &row) -> int64_t {
                      return static_cast<int64_t>(row.ea);
                    })
      .column_text_rw(
          "comment",
          [](const CommentRow &row) -> std::string { return row.comment; },
          [](CommentRow &row, const char *new_cmt) -> bool {
            const std::string requested = new_cmt ? new_cmt : "";
            // Idempotent: read current comment from IDA to skip no-ops
            // (row_populator means row.comment has argv values, not IDA state)
            qstring cur;
            get_cmt(&cur, row.ea, false);
            if (requested == std::string(cur.c_str())) return true;
            idasql_auto_wait();
            bool ok = set_cmt(row.ea, requested.c_str(), false);
            if (ok)
              row.comment = requested;
            else
              xsql::set_vtab_error("comments: failed to set comment at " +
                                   idasql::format_ea_hex(row.ea));
            idasql_auto_wait();
            return ok;
          })
      .column_text_rw(
          "rpt_comment",
          [](const CommentRow &row) -> std::string { return row.rpt_comment; },
          [](CommentRow &row, const char *new_cmt) -> bool {
            const std::string requested = new_cmt ? new_cmt : "";
            qstring cur;
            get_cmt(&cur, row.ea, true);
            if (requested == std::string(cur.c_str())) return true;
            idasql_auto_wait();
            bool ok = set_cmt(row.ea, requested.c_str(), true);
            if (ok)
              row.rpt_comment = requested;
            else
              xsql::set_vtab_error("comments: failed to set repeatable comment at " +
                                   idasql::format_ea_hex(row.ea));
            idasql_auto_wait();
            return ok;
          })
      .deletable([](CommentRow &row) -> bool {
        idasql_auto_wait();
        set_cmt(row.ea, "", false);
        set_cmt(row.ea, "", true);
        idasql_auto_wait();
        return true;
      })
      .insertable([](int argc, xsql::FunctionArg *argv) -> bool {
        if (argc < 1 || argv[0].is_null())
          return false;

        ea_t ea = static_cast<ea_t>(argv[0].as_int64());
        bool did_something = false;
        idasql_auto_wait();
        if (argc > 1 && !argv[1].is_null()) {
          const char *cmt = argv[1].as_c_str();
          if (cmt) {
            set_cmt(ea, cmt, false);
            did_something = true;
          }
        }
        if (argc > 2 && !argv[2].is_null()) {
          const char *rpt = argv[2].as_c_str();
          if (rpt) {
            set_cmt(ea, rpt, true);
            did_something = true;
          }
        }

        idasql_auto_wait();
        return did_something;
      })
      .build();
}

} // namespace symbols
} // namespace idasql
