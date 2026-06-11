// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "memory_segments.hpp"

using namespace idasql::core;

namespace idasql {
namespace memory {

// ============================================================================
// SEGMENTS Table
// ============================================================================

VTableDef define_segments() {
  return table("segments")
      .count([]() { return static_cast<size_t>(get_segm_qty()); })
      .column_int64("start_ea",
                    [](size_t i) -> int64_t {
                      segment_t *s = getnseg(static_cast<int>(i));
                      return s ? static_cast<int64_t>(s->start_ea) : 0;
                    })
      .column_int64("end_ea",
                    [](size_t i) -> int64_t {
                      segment_t *s = getnseg(static_cast<int>(i));
                      return s ? static_cast<int64_t>(s->end_ea) : 0;
                    })
      .column_text_rw(
          "name",
          // Getter
          [](size_t i) -> std::string {
            segment_t *s = getnseg(static_cast<int>(i));
            return safe_segm_name(s);
          },
          // Setter - rename segment
          [](size_t i, const char *new_name) -> bool {
            idasql_auto_wait();
            segment_t *s = getnseg(static_cast<int>(i));
            if (!s) {
              xsql::set_vtab_error("segments: segment not found at index " + std::to_string(i));
              return false;
            }
            bool ok = set_segm_name(s, new_name) != 0;
            if (!ok)
              xsql::set_vtab_error("segments: failed to rename segment " +
                                   idasql::format_ea_hex(s->start_ea));
            idasql_auto_wait();
            return ok;
          })
      .column_text_rw(
          "class",
          // Getter
          [](size_t i) -> std::string {
            segment_t *s = getnseg(static_cast<int>(i));
            return safe_segm_class(s);
          },
          // Setter - change segment class
          [](size_t i, const char *new_class) -> bool {
            idasql_auto_wait();
            segment_t *s = getnseg(static_cast<int>(i));
            if (!s) {
              xsql::set_vtab_error("segments: segment not found at index " + std::to_string(i));
              return false;
            }
            bool ok = set_segm_class(s, new_class) != 0;
            if (!ok)
              xsql::set_vtab_error("segments: failed to set class on segment " +
                                   idasql::format_ea_hex(s->start_ea));
            idasql_auto_wait();
            return ok;
          })
      .column_int_rw(
          "perm",
          // Getter
          [](size_t i) -> int {
            segment_t *s = getnseg(static_cast<int>(i));
            return s ? s->perm : 0;
          },
          // Setter - change segment permissions
          [](size_t i, int new_perm) -> bool {
            idasql_auto_wait();
            segment_t *s = getnseg(static_cast<int>(i));
            if (!s) {
              xsql::set_vtab_error("segments: segment not found at index " + std::to_string(i));
              return false;
            }
            s->perm = static_cast<uchar>(new_perm);
            bool ok = s->update();
            if (!ok)
              xsql::set_vtab_error("segments: failed to update permissions on segment " +
                                   idasql::format_ea_hex(s->start_ea));
            idasql_auto_wait();
            return ok;
          })
      .deletable([](size_t i) -> bool {
        idasql_auto_wait();
        segment_t *s = getnseg(static_cast<int>(i));
        if (!s)
          return false;
        bool ok = del_segm(s->start_ea, SEGMOD_KILL) != 0;
        idasql_auto_wait();
        return ok;
      })
      .insertable([](int argc, xsql::FunctionArg *argv) -> bool {
        // start_ea (col 0) and end_ea (col 1) are required
        if (argc < 2 || argv[0].is_null() || argv[1].is_null())
          return false;

        ea_t start = static_cast<ea_t>(argv[0].as_int64());
        ea_t end = static_cast<ea_t>(argv[1].as_int64());
        if (start == BADADDR || end == BADADDR || end <= start)
          return false;

        int perm = 0;
        bool has_perm = false;
        if (argc > 4 && !argv[4].is_null()) {
          perm = argv[4].as_int();
          if (perm < 0 || perm > 7)
            return false;
          has_perm = true;
        }

        // Avoid destructive overlap behavior from add_segm().
        const int seg_qty = get_segm_qty();
        for (int seg_idx = 0; seg_idx < seg_qty; ++seg_idx) {
          segment_t *seg = getnseg(seg_idx);
          if (!seg)
            continue;
          if (start < seg->end_ea && end > seg->start_ea) {
            return false;
          }
        }

        const char *seg_name = nullptr;
        if (argc > 2 && !argv[2].is_null()) {
          seg_name = argv[2].as_c_str();
          if (seg_name && seg_name[0] == '\0')
            seg_name = nullptr;
        }

        const char *seg_class = nullptr;
        if (argc > 3 && !argv[3].is_null()) {
          seg_class = argv[3].as_c_str();
          if (seg_class && seg_class[0] == '\0')
            seg_class = nullptr;
        }

        const ea_t para = start >> 4;

        idasql_auto_wait();
        bool ok = add_segm(para, start, end, seg_name, seg_class,
                           ADDSEG_QUIET | ADDSEG_NOAA);
        if (ok && has_perm) {
          segment_t *created = getseg(start);
          if (created == nullptr) {
            ok = false;
          } else {
            created->perm = static_cast<uchar>(perm);
            ok = created->update();
          }
        }
        idasql_auto_wait();
        return ok;
      })
      .build();
}

} // namespace memory
} // namespace idasql
