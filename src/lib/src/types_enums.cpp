// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "types_enums.hpp"

namespace idasql {
namespace types {

void collect_enum_values(std::vector<EnumValueEntry>& rows) {
    rows.clear();

    til_t* ti = get_idati();
    if (!ti) return;

    uint32_t max_ord = get_ordinal_limit(ti);
    if (max_ord == 0 || max_ord == uint32_t(-1)) return;

    for (uint32_t ord = 1; ord < max_ord; ++ord) {
        const char* name = get_numbered_type_name(ti, ord);
        if (!name) continue;  // Skip gaps in ordinal space

        tinfo_t tif;
        if (tif.get_numbered_type(ti, ord)) {
            if (tif.is_enum()) {
                enum_type_data_t ei;
                if (tif.get_enum_details(&ei)) {
                    for (size_t i = 0; i < ei.size(); i++) {
                        const edm_t& e = ei[i];
                        EnumValueEntry entry;
                        entry.type_ordinal = ord;
                        entry.type_name = name;
                        entry.value_index = static_cast<int>(i);
                        entry.value_name = e.name.c_str();
                        entry.value = static_cast<int64_t>(e.value);
                        entry.uvalue = e.value;
                        entry.comment = e.cmt.c_str();
                        rows.push_back(std::move(entry));
                    }
                }
            }
        }
    }
}

// ============================================================================
// EnumValuesInTypeIterator
// ============================================================================

EnumValuesInTypeIterator::EnumValuesInTypeIterator(uint32_t ordinal) : type_ordinal_(ordinal) {
    til_t* ti = get_idati();
    if (!ti) return;

    const char* name = get_numbered_type_name(ti, type_ordinal_);
    if (!name) return;
    type_name_ = name;

    tinfo_t tif;
    if (tif.get_numbered_type(ti, type_ordinal_)) {
        if (tif.is_enum()) {
            has_data_ = tif.get_enum_details(&ei_);
        }
    }
}

bool EnumValuesInTypeIterator::next() {
    if (!has_data_) return false;
    ++idx_;
    valid_ = (idx_ >= 0 && static_cast<size_t>(idx_) < ei_.size());
    return valid_;
}

bool EnumValuesInTypeIterator::eof() const {
    return idx_ >= 0 && !valid_;
}

void EnumValuesInTypeIterator::column(xsql::FunctionContext& ctx, int col) {
    if (!valid_ || idx_ < 0 || static_cast<size_t>(idx_) >= ei_.size()) {
        ctx.result_null();
        return;
    }
    const edm_t& e = ei_[idx_];
    switch (col) {
        case 0: ctx.result_int(type_ordinal_); break;
        case 1: ctx.result_text(type_name_.c_str()); break;
        case 2: ctx.result_int(idx_); break;
        case 3: ctx.result_text(e.name.c_str()); break;
        case 4: ctx.result_int64(static_cast<int64_t>(e.value)); break;
        case 5: ctx.result_int64(static_cast<int64_t>(e.value)); break;  // uvalue
        case 6: ctx.result_text(e.cmt.c_str()); break;
        default: ctx.result_null(); break;
    }
}

int64_t EnumValuesInTypeIterator::rowid() const {
    return static_cast<int64_t>(type_ordinal_) * 10000 + idx_;
}

// ============================================================================
// EnumTypeRef
// ============================================================================

EnumTypeRef::EnumTypeRef(uint32_t ord) : valid(false), ordinal(ord) {
    til_t* ti = get_idati();
    if (!ti) return;
    if (tif.get_numbered_type(ti, ord)) {
        if (tif.is_enum()) {
            valid = tif.get_enum_details(&ei);
        }
    }
}

bool EnumTypeRef::save() {
    if (!valid) return false;
    tinfo_t new_tif;
    new_tif.create_enum(ei);
    return new_tif.set_numbered_type(get_idati(), ordinal, NTF_REPLACE, nullptr);
}

// ============================================================================
// build_enum_value_entry
// ============================================================================

bool build_enum_value_entry(uint32_t ordinal, int value_index, EnumValueEntry& entry) {
    til_t* ti = get_idati();
    if (!ti) return false;
    const char* type_name = get_numbered_type_name(ti, ordinal);
    if (!type_name) return false;

    tinfo_t tif;
    if (!tif.get_numbered_type(ti, ordinal)) return false;
    if (!tif.is_enum()) return false;

    enum_type_data_t ei;
    if (!tif.get_enum_details(&ei)) return false;
    if (value_index < 0 || static_cast<size_t>(value_index) >= ei.size()) return false;

    const edm_t& e = ei[value_index];
    entry.type_ordinal = ordinal;
    entry.type_name = type_name;
    entry.value_index = value_index;
    entry.value_name = e.name.c_str();
    entry.value = static_cast<int64_t>(e.value);
    entry.uvalue = e.value;
    entry.comment = e.cmt.c_str();
    return true;
}

// ============================================================================
// TYPES_ENUM_VALUES Table Definition
// ============================================================================

CachedTableDef<EnumValueEntry> define_types_enum_values() {
    return cached_table<EnumValueEntry>("types_enum_values")
        .no_shared_cache()
        .estimate_rows([]() -> size_t {
            til_t* ti = get_idati();
            return ti ? static_cast<size_t>(get_ordinal_limit(ti)) * 8 : 0;
        })
        .cache_builder([](std::vector<EnumValueEntry>& rows) {
            collect_enum_values(rows);
        })
        .row_populator([](EnumValueEntry& row, int argc, xsql::FunctionArg* argv) {
            if (argc > 2 && !argv[2].is_null()) row.type_ordinal = static_cast<uint32_t>(argv[2].as_int());
            if (argc > 4 && !argv[4].is_null()) row.value_index = argv[4].as_int();
        })
        .row_lookup([](EnumValueEntry& row, int64_t rowid) -> bool {
            if (rowid < 0) return false;
            uint32_t ordinal = static_cast<uint32_t>(rowid / 10000);
            int value_index = static_cast<int>(rowid % 10000);
            return build_enum_value_entry(ordinal, value_index, row);
        })
        .column_int("type_ordinal", [](const EnumValueEntry& row) -> int {
            return static_cast<int>(row.type_ordinal);
        })
        .column_text("type_name", [](const EnumValueEntry& row) -> std::string {
            return row.type_name;
        })
        .column_int("value_index", [](const EnumValueEntry& row) -> int {
            return row.value_index;
        })
        .column_text_rw("value_name",
            [](const EnumValueEntry& row) -> std::string {
                return row.value_name;
            },
            [](EnumValueEntry& row, const char* new_name) -> bool {
                const std::string ctx = "enum=" + row.type_name + " value_index=" + std::to_string(row.value_index);
                EnumTypeRef ref(row.type_ordinal);
                if (!ref.valid) {
                    xsql::set_vtab_error("types_enum_values: enum type not found (" + ctx + ")");
                    return false;
                }
                if (row.value_index < 0 || static_cast<size_t>(row.value_index) >= ref.ei.size()) {
                    xsql::set_vtab_error("types_enum_values: value index out of range (" + ctx + ")");
                    return false;
                }
                ref.ei[row.value_index].name = new_name ? new_name : "";
                bool ok = ref.save();
                if (ok) row.value_name = new_name ? new_name : "";
                else xsql::set_vtab_error("types_enum_values: failed to save (" + ctx + ")");
                return ok;
            })
        .column_int64_rw("value",
            [](const EnumValueEntry& row) -> int64_t {
                return row.value;
            },
            [](EnumValueEntry& row, int64_t new_value) -> bool {
                const std::string ctx = "enum=" + row.type_name + " value_index=" + std::to_string(row.value_index);
                EnumTypeRef ref(row.type_ordinal);
                if (!ref.valid) {
                    xsql::set_vtab_error("types_enum_values: enum type not found (" + ctx + ")");
                    return false;
                }
                if (row.value_index < 0 || static_cast<size_t>(row.value_index) >= ref.ei.size()) {
                    xsql::set_vtab_error("types_enum_values: value index out of range (" + ctx + ")");
                    return false;
                }
                ref.ei[row.value_index].value = static_cast<uint64_t>(new_value);
                bool ok = ref.save();
                if (ok) {
                    row.value = new_value;
                    row.uvalue = static_cast<uint64_t>(new_value);
                } else {
                    xsql::set_vtab_error("types_enum_values: failed to save (" + ctx + ")");
                }
                return ok;
            })
        .column_int64("uvalue", [](const EnumValueEntry& row) -> int64_t {
            return static_cast<int64_t>(row.uvalue);
        })
        .column_text_rw("comment",
            [](const EnumValueEntry& row) -> std::string {
                return row.comment;
            },
            [](EnumValueEntry& row, const char* new_comment) -> bool {
                const std::string ctx = "enum=" + row.type_name + " value_index=" + std::to_string(row.value_index);
                EnumTypeRef ref(row.type_ordinal);
                if (!ref.valid) {
                    xsql::set_vtab_error("types_enum_values: enum type not found (" + ctx + ")");
                    return false;
                }
                if (row.value_index < 0 || static_cast<size_t>(row.value_index) >= ref.ei.size()) {
                    xsql::set_vtab_error("types_enum_values: value index out of range (" + ctx + ")");
                    return false;
                }
                ref.ei[row.value_index].cmt = new_comment ? new_comment : "";
                bool ok = ref.save();
                if (ok) row.comment = new_comment ? new_comment : "";
                else xsql::set_vtab_error("types_enum_values: failed to save (" + ctx + ")");
                return ok;
            })
        .deletable([](EnumValueEntry& row) -> bool {
            EnumTypeRef ref(row.type_ordinal);
            if (!ref.valid) return false;
            if (row.value_index < 0 || static_cast<size_t>(row.value_index) >= ref.ei.size()) return false;
            ref.ei.erase(ref.ei.begin() + row.value_index);
            return ref.save();
        })
        .insertable([](int argc, xsql::FunctionArg* argv) -> bool {
            if (argc < 4
                || argv[0].is_null()
                || argv[3].is_null())
                return false;

            uint32_t ordinal = static_cast<uint32_t>(argv[0].as_int());
            const char* value_name = argv[3].as_c_str();
            if (!value_name || !value_name[0]) return false;

            EnumTypeRef ref(ordinal);
            if (!ref.valid) return false;

            edm_t new_edm;
            new_edm.name = value_name;
            if (argc > 4 && !argv[4].is_null()) {
                new_edm.value = static_cast<uint64_t>(argv[4].as_int64());
            } else {
                if (!ref.ei.empty()) {
                    new_edm.value = ref.ei.back().value + 1;
                } else {
                    new_edm.value = 0;
                }
            }
            if (argc > 6 && !argv[6].is_null()) {
                const char* cmt = argv[6].as_c_str();
                if (cmt) new_edm.cmt = cmt;
            }

            ref.ei.push_back(new_edm);
            return ref.save();
        })
        .filter_eq("type_ordinal", [](int64_t ordinal) -> std::unique_ptr<xsql::RowIterator> {
            return std::make_unique<EnumValuesInTypeIterator>(static_cast<uint32_t>(ordinal));
        }, 10.0, 10.0)
        .build();
}

} // namespace types
} // namespace idasql
