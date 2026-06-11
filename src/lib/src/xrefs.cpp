// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "xrefs.hpp"

using namespace idasql::core;

namespace idasql {
namespace xrefs {

// ============================================================================
// Xref Iterators
// ============================================================================

XrefsToIterator::XrefsToIterator(ea_t target) : target_(target) {}

bool XrefsToIterator::next() {
  if (!started_) {
    started_ = true;
    valid_ = xb_.first_to(target_, XREF_ALL);
  } else if (valid_) {
    valid_ = xb_.next_to();
  }
  return valid_;
}

bool XrefsToIterator::eof() const { return started_ && !valid_; }

void XrefsToIterator::column(xsql::FunctionContext &ctx, int col) {
  if (!valid_) {
    ctx.result_null();
    return;
  }
  switch (col) {
  case 0:
    ctx.result_int64(static_cast<int64_t>(xb_.from));
    break;
  case 1:
    ctx.result_int64(static_cast<int64_t>(target_));
    break;
  case 2: {
    func_t *f = get_func(xb_.from);
    ctx.result_int64(f ? static_cast<int64_t>(f->start_ea) : 0);
    break;
  }
  case 3:
    ctx.result_int(xb_.type);
    break;
  case 4:
    ctx.result_int(xb_.iscode ? 1 : 0);
    break;
  default:
    ctx.result_null();
    break;
  }
}

int64_t XrefsToIterator::rowid() const {
  return valid_ ? static_cast<int64_t>(xb_.from) : 0;
}

XrefsFromIterator::XrefsFromIterator(ea_t source) : source_(source) {}

bool XrefsFromIterator::next() {
  if (!started_) {
    started_ = true;
    valid_ = xb_.first_from(source_, XREF_ALL);
  } else if (valid_) {
    valid_ = xb_.next_from();
  }
  return valid_;
}

bool XrefsFromIterator::eof() const { return started_ && !valid_; }

void XrefsFromIterator::column(xsql::FunctionContext &ctx, int col) {
  if (!valid_) {
    ctx.result_null();
    return;
  }
  switch (col) {
  case 0:
    ctx.result_int64(static_cast<int64_t>(source_));
    break;
  case 1:
    ctx.result_int64(static_cast<int64_t>(xb_.to));
    break;
  case 2: {
    func_t *f = get_func(source_);
    ctx.result_int64(f ? static_cast<int64_t>(f->start_ea) : 0);
    break;
  }
  case 3:
    ctx.result_int(xb_.type);
    break;
  case 4:
    ctx.result_int(xb_.iscode ? 1 : 0);
    break;
  default:
    ctx.result_null();
    break;
  }
}

int64_t XrefsFromIterator::rowid() const {
  return valid_ ? static_cast<int64_t>(xb_.to) : 0;
}

// ============================================================================
// XrefsFromFuncIterator - all xrefs originating from within a function
// ============================================================================

XrefsFromFuncIterator::XrefsFromFuncIterator(ea_t func_ea) : func_ea_(func_ea) {
  func_t *pfn = get_func(func_ea);
  if (pfn) {
    fii_valid_ = fii_.set(pfn);
  }
}

bool XrefsFromFuncIterator::advance_to_next_xref() {
  // Try next xref from current item
  if (xb_valid_) {
    xb_valid_ = xb_.next_from();
    if (xb_valid_)
      return true;
  }

  // Advance to next item in function that has xrefs
  while (fii_valid_) {
    ea_t item_ea = fii_.current();
    fii_valid_ = fii_.next_code();

    xb_valid_ = xb_.first_from(item_ea, XREF_ALL);
    if (xb_valid_)
      return true;
  }

  return false;
}

bool XrefsFromFuncIterator::next() {
  if (!started_) {
    started_ = true;
    if (!fii_valid_) {
      eof_ = true;
      return false;
    }
    // Position on first item
    ea_t item_ea = fii_.current();
    fii_valid_ = fii_.next_code();
    xb_valid_ = xb_.first_from(item_ea, XREF_ALL);
    if (xb_valid_)
      return true;
    // No xrefs from first item, try subsequent
    if (advance_to_next_xref())
      return true;
    eof_ = true;
    return false;
  }

  if (advance_to_next_xref())
    return true;
  eof_ = true;
  return false;
}

bool XrefsFromFuncIterator::eof() const { return eof_; }

void XrefsFromFuncIterator::column(xsql::FunctionContext &ctx, int col) {
  if (!xb_valid_) {
    ctx.result_null();
    return;
  }
  switch (col) {
  case 0:
    ctx.result_int64(static_cast<int64_t>(xb_.from));
    break;
  case 1:
    ctx.result_int64(static_cast<int64_t>(xb_.to));
    break;
  case 2:
    ctx.result_int64(static_cast<int64_t>(func_ea_));
    break;
  case 3:
    ctx.result_int(xb_.type);
    break;
  case 4:
    ctx.result_int(xb_.iscode ? 1 : 0);
    break;
  default:
    ctx.result_null();
    break;
  }
}

int64_t XrefsFromFuncIterator::rowid() const {
  return xb_valid_ ? static_cast<int64_t>(xb_.from) : 0;
}

// ============================================================================
// XREFS Table
// ============================================================================

CachedTableDef<XrefInfo> define_xrefs() {
  return cached_table<XrefInfo>("xrefs")
      .no_shared_cache()
      // Estimate row count without building cache
      .estimate_rows([]() -> size_t {
        // Heuristic: ~10 xrefs per function on average
        return get_func_qty() * 10;
      })
      // Cache builder (called lazily, only if pushdown doesn't handle query)
      .cache_builder([](std::vector<XrefInfo> &cache) {
        size_t func_qty = get_func_qty();
        for (size_t i = 0; i < func_qty; i++) {
          func_t *func = getn_func(i);
          if (!func)
            continue;

          // Xrefs TO this function
          xrefblk_t xb;
          for (bool ok = xb.first_to(func->start_ea, XREF_ALL); ok;
               ok = xb.next_to()) {
            XrefInfo xi;
            xi.from_ea = xb.from;
            xi.to_ea = func->start_ea;
            xi.type = xb.type;
            xi.is_code = xb.iscode;
            // Pre-compute containing function for from_ea
            func_t *from_fn = get_func(xb.from);
            xi.from_func = from_fn ? from_fn->start_ea : BADADDR;
            cache.push_back(xi);
          }
        }
      })
      // Column order: from_ea, to_ea, from_func, type, is_code (matches bnsql)
      .column_int64("from_ea",
                    [](const XrefInfo &r) -> int64_t {
                      return static_cast<int64_t>(r.from_ea);
                    })
      .column_int64("to_ea",
                    [](const XrefInfo &r) -> int64_t {
                      return static_cast<int64_t>(r.to_ea);
                    })
      .column_int64("from_func",
                    [](const XrefInfo &r) -> int64_t {
                      return r.from_func != BADADDR
                                 ? static_cast<int64_t>(r.from_func)
                                 : 0;
                    })
      .column_int(
          "type",
          [](const XrefInfo &r) -> int { return static_cast<int>(r.type); })
      .column_int("is_code",
                  [](const XrefInfo &r) -> int { return r.is_code ? 1 : 0; })
      // Constraint pushdown: native IDA iterators bypass cache for O(1) lookups
      .filter_eq(
          "to_ea",
          [](int64_t target) -> std::unique_ptr<xsql::RowIterator> {
            return std::make_unique<XrefsToIterator>(static_cast<ea_t>(target));
          },
          0.5, 5.0)
      .filter_eq(
          "from_ea",
          [](int64_t source) -> std::unique_ptr<xsql::RowIterator> {
            return std::make_unique<XrefsFromIterator>(
                static_cast<ea_t>(source));
          },
          0.5, 5.0)
      .filter_eq(
          "from_func",
          [](int64_t func_addr) -> std::unique_ptr<xsql::RowIterator> {
            return std::make_unique<XrefsFromFuncIterator>(
                static_cast<ea_t>(func_addr));
          },
          1.0, 10.0)
      .build();
}

CachedTableDef<DataRefInfo> define_data_refs() {
  return cached_table<DataRefInfo>("data_refs")
      .no_shared_cache()
      .estimate_rows([]() -> size_t { return get_func_qty() * 4; })
      .cache_builder([](std::vector<DataRefInfo> &cache) {
        size_t func_qty = get_func_qty();
        for (size_t i = 0; i < func_qty; i++) {
          func_t *func = getn_func(i);
          if (!func)
            continue;

          func_item_iterator_t fii;
          if (!fii.set(func))
            continue;

          auto collect_item_refs = [&](ea_t item_ea) {
            xrefblk_t xb;
            for (bool ok = xb.first_from(item_ea, XREF_ALL); ok;
                 ok = xb.next_from()) {
              if (xb.iscode || xb.to == BADADDR)
                continue;

              DataRefInfo row;
              row.from_ea = xb.from;
              row.to_ea = xb.to;
              row.from_func = func->start_ea;
              row.type = xb.type;
              cache.push_back(row);
            }
          };

          collect_item_refs(fii.current());
          while (fii.next_code()) {
            collect_item_refs(fii.current());
          }
        }
      })
      .column_int64("from_addr",
                    [](const DataRefInfo &row) -> int64_t {
                      return static_cast<int64_t>(row.from_ea);
                    })
      .column_int64("to_addr",
                    [](const DataRefInfo &row) -> int64_t {
                      return static_cast<int64_t>(row.to_ea);
                    })
      .column_int64("from_func_addr",
                    [](const DataRefInfo &row) -> int64_t {
                      return row.from_func != BADADDR
                                 ? static_cast<int64_t>(row.from_func)
                                 : 0;
                    })
      .column_text("from_func_name",
                   [](const DataRefInfo &row) -> std::string {
                     return row.from_func != BADADDR
                                ? name_or_fallback(row.from_func)
                                : "";
                   })
      .column_int("ref_type",
                  [](const DataRefInfo &row) -> int {
                    return static_cast<int>(row.type);
                  })
      .build();
}

} // namespace xrefs
} // namespace idasql
