// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "types_members.hpp"

#include <cctype>

namespace idasql {
namespace types {

namespace {

// Parse types_members type text, including suffix array forms such as char[256].
bool parse_member_type(const std::string& type_text, tinfo_t& out_type) {
    const std::string type = trim_copy(type_text);
    if (type.empty()) return false;

    size_t type_end = type.size();
    std::string array_suffix;
    while (type_end > 0 && type[type_end - 1] == ']') {
        size_t depth = 0;
        size_t open = type_end;
        for (size_t i = type_end; i-- > 0;) {
            if (type[i] == ']') {
                ++depth;
            } else if (type[i] == '[' && --depth == 0) {
                open = i;
                break;
            }
        }
        if (open == type_end) return false;
        array_suffix.insert(0, type.substr(open, type_end - open));
        type_end = open;
        while (type_end > 0 && std::isspace(static_cast<unsigned char>(type[type_end - 1]))) {
            --type_end;
        }
    }

    const std::string base_type = trim_copy(type.substr(0, type_end));
    if (base_type.empty()) return false;

    const std::string declaration = base_type + " __idasql_member" + array_suffix + ";";
    qstring parsed_name;
    return parse_decl(&out_type, &parsed_name, nullptr, declaration.c_str(), PT_SIL);
}

void refresh_member_type_fields(MemberEntry& entry, const udm_t& member, til_t* ti) {
    entry.size = static_cast<int64_t>(member.size / 8);
    entry.size_bits = static_cast<int64_t>(member.size);

    qstring type_str;
    member.type.print(&type_str);
    entry.member_type = type_str.c_str();
    classify_member_type(member.type, ti,
        entry.mt_is_struct, entry.mt_is_union, entry.mt_is_enum,
        entry.mt_is_ptr, entry.mt_is_array, entry.member_type_ordinal);
}

} // namespace

int get_type_ordinal_by_name(til_t* ti, const char* type_name) {
    if (!ti || !type_name || !type_name[0]) return -1;
    uint32_t ord = get_type_ordinal(ti, type_name);
    return (ord != 0) ? static_cast<int>(ord) : -1;
}

void classify_member_type(const tinfo_t& mtype, til_t* ti,
                          bool& is_struct, bool& is_union, bool& is_enum,
                          bool& is_ptr, bool& is_array, int& type_ordinal) {
    is_struct = false;
    is_union = false;
    is_enum = false;
    is_ptr = mtype.is_ptr();
    is_array = mtype.is_array();
    type_ordinal = -1;

    // Get the base type (dereference pointers/arrays to find underlying type)
    tinfo_t base_type = mtype;
    if (mtype.is_ptr()) {
        base_type = mtype.get_pointed_object();
    } else if (mtype.is_array()) {
        base_type = mtype.get_array_element();
    }

    // Classify the base type
    is_struct = base_type.is_struct();
    is_union = base_type.is_union();
    is_enum = base_type.is_enum();

    // Try to get ordinal of the base type
    qstring type_name;
    if (base_type.get_type_name(&type_name) && !type_name.empty()) {
        type_ordinal = get_type_ordinal_by_name(ti, type_name.c_str());
    }
}

void collect_members(std::vector<MemberEntry>& rows) {
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
            if (tif.is_struct() || tif.is_union()) {
                udt_type_data_t udt;
                if (tif.get_udt_details(&udt)) {
                    for (size_t i = 0; i < udt.size(); i++) {
                        const udm_t& m = udt[i];
                        MemberEntry entry;
                        entry.type_ordinal = ord;
                        entry.type_name = name;
                        entry.member_index = static_cast<int>(i);
                        entry.member_name = m.name.c_str();
                        entry.offset = static_cast<int64_t>(m.offset / 8);
                        entry.offset_bits = static_cast<int64_t>(m.offset);
                        entry.size = static_cast<int64_t>(m.size / 8);
                        entry.size_bits = static_cast<int64_t>(m.size);
                        entry.is_bitfield = m.is_bitfield();
                        entry.is_baseclass = m.is_baseclass();
                        entry.comment = m.cmt.c_str();

                        qstring type_str;
                        m.type.print(&type_str);
                        entry.member_type = type_str.c_str();

                        // Classify member type
                        classify_member_type(m.type, ti,
                            entry.mt_is_struct, entry.mt_is_union, entry.mt_is_enum,
                            entry.mt_is_ptr, entry.mt_is_array, entry.member_type_ordinal);

                        rows.push_back(std::move(entry));
                    }
                }
            }
        }
    }
}

// ============================================================================
// MembersInTypeIterator
// ============================================================================

MembersInTypeIterator::MembersInTypeIterator(uint32_t ordinal) : type_ordinal_(ordinal) {
    til_t* ti = get_idati();
    if (!ti) return;

    const char* name = get_numbered_type_name(ti, type_ordinal_);
    if (!name) return;
    type_name_ = name;

    tinfo_t tif;
    if (tif.get_numbered_type(ti, type_ordinal_)) {
        if (tif.is_struct() || tif.is_union()) {
            has_data_ = tif.get_udt_details(&udt_);
        }
    }
}

bool MembersInTypeIterator::next() {
    if (!has_data_) return false;
    ++idx_;
    valid_ = (idx_ >= 0 && static_cast<size_t>(idx_) < udt_.size());
    return valid_;
}

bool MembersInTypeIterator::eof() const {
    return idx_ >= 0 && !valid_;
}

void MembersInTypeIterator::column(xsql::FunctionContext& ctx, int col) {
    if (!valid_ || idx_ < 0 || static_cast<size_t>(idx_) >= udt_.size()) {
        ctx.result_null();
        return;
    }
    const udm_t& m = udt_[idx_];
    switch (col) {
        case 0: ctx.result_int(type_ordinal_); break;
        case 1: ctx.result_text(type_name_.c_str()); break;
        case 2: ctx.result_int(idx_); break;
        case 3: ctx.result_text(m.name.c_str()); break;
        case 4: ctx.result_int64(static_cast<int64_t>(m.offset / 8)); break;
        case 5: ctx.result_int64(static_cast<int64_t>(m.offset)); break;
        case 6: ctx.result_int64(static_cast<int64_t>(m.size / 8)); break;
        case 7: ctx.result_int64(static_cast<int64_t>(m.size)); break;
        case 8: {
            qstring type_str;
            m.type.print(&type_str);
            ctx.result_text(type_str.c_str());
            break;
        }
        case 9: ctx.result_int(m.is_bitfield() ? 1 : 0); break;
        case 10: ctx.result_int(m.is_baseclass() ? 1 : 0); break;
        case 11: ctx.result_text(m.cmt.c_str()); break;
        // Member type classification columns
        case 12: case 13: case 14: case 15: case 16: case 17: {
            // Classify the member type on-the-fly for iterator
            bool mt_is_struct, mt_is_union, mt_is_enum, mt_is_ptr, mt_is_array;
            int mt_ordinal;
            classify_member_type(m.type, get_idati(),
                mt_is_struct, mt_is_union, mt_is_enum,
                mt_is_ptr, mt_is_array, mt_ordinal);
            switch (col) {
                case 12: ctx.result_int(mt_is_struct ? 1 : 0); break;
                case 13: ctx.result_int(mt_is_union ? 1 : 0); break;
                case 14: ctx.result_int(mt_is_enum ? 1 : 0); break;
                case 15: ctx.result_int(mt_is_ptr ? 1 : 0); break;
                case 16: ctx.result_int(mt_is_array ? 1 : 0); break;
                case 17: ctx.result_int(mt_ordinal); break;
            }
            break;
        }
        default: ctx.result_null(); break;
    }
}

int64_t MembersInTypeIterator::rowid() const {
    return static_cast<int64_t>(type_ordinal_) * 10000 + idx_;
}

// ============================================================================
// TypeMemberRef
// ============================================================================

TypeMemberRef::TypeMemberRef(uint32_t ord) : valid(false), ordinal(ord) {
    til_t* ti = get_idati();
    if (!ti) return;
    if (tif.get_numbered_type(ti, ord)) {
        if (tif.is_struct() || tif.is_union()) {
            valid = tif.get_udt_details(&udt);
        }
    }
}

bool TypeMemberRef::save() {
    if (!valid) return false;
    tinfo_t new_tif;
    if (!new_tif.create_udt(udt)) {
        return false;
    }
    return new_tif.set_numbered_type(get_idati(), ordinal, NTF_REPLACE, nullptr) == TERR_OK;
}

// ============================================================================
// build_member_entry
// ============================================================================

bool build_member_entry(uint32_t ordinal, int member_index, MemberEntry& entry) {
    til_t* ti = get_idati();
    if (!ti) return false;

    const char* type_name = get_numbered_type_name(ti, ordinal);
    if (!type_name) return false;

    tinfo_t tif;
    if (!tif.get_numbered_type(ti, ordinal)) return false;
    if (!(tif.is_struct() || tif.is_union())) return false;

    udt_type_data_t udt;
    if (!tif.get_udt_details(&udt)) return false;
    if (member_index < 0 || static_cast<size_t>(member_index) >= udt.size()) return false;

    const udm_t& m = udt[member_index];
    entry.type_ordinal = ordinal;
    entry.type_name = type_name;
    entry.member_index = member_index;
    entry.member_name = m.name.c_str();
    entry.offset = static_cast<int64_t>(m.offset / 8);
    entry.offset_bits = static_cast<int64_t>(m.offset);
    entry.size = static_cast<int64_t>(m.size / 8);
    entry.size_bits = static_cast<int64_t>(m.size);
    entry.is_bitfield = m.is_bitfield();
    entry.is_baseclass = m.is_baseclass();
    entry.comment = m.cmt.c_str();

    qstring type_str;
    m.type.print(&type_str);
    entry.member_type = type_str.c_str();

    classify_member_type(m.type, ti,
        entry.mt_is_struct, entry.mt_is_union, entry.mt_is_enum,
        entry.mt_is_ptr, entry.mt_is_array, entry.member_type_ordinal);
    return true;
}

// ============================================================================
// TYPES_MEMBERS Table Definition
// ============================================================================

CachedTableDef<MemberEntry> define_types_members() {
    return cached_table<MemberEntry>("types_members")
        .no_shared_cache()
        .estimate_rows([]() -> size_t {
            til_t* ti = get_idati();
            return ti ? static_cast<size_t>(get_ordinal_limit(ti)) * 8 : 0;
        })
        .cache_builder([](std::vector<MemberEntry>& rows) {
            collect_members(rows);
        })
        .row_populator([](MemberEntry& row, int argc, xsql::FunctionArg* argv) {
            if (argc > 2 && !argv[2].is_null()) row.type_ordinal = static_cast<uint32_t>(argv[2].as_int());
            if (argc > 4 && !argv[4].is_null()) row.member_index = argv[4].as_int();
        })
        .row_lookup([](MemberEntry& row, int64_t rowid) -> bool {
            if (rowid < 0) return false;
            uint32_t ordinal = static_cast<uint32_t>(rowid / 10000);
            int member_index = static_cast<int>(rowid % 10000);
            return build_member_entry(ordinal, member_index, row);
        })
        .column_int("type_ordinal", [](const MemberEntry& row) -> int {
            return static_cast<int>(row.type_ordinal);
        })
        .column_text("type_name", [](const MemberEntry& row) -> std::string {
            return row.type_name;
        })
        .column_int("member_index", [](const MemberEntry& row) -> int {
            return row.member_index;
        })
        .column_text_rw("member_name",
            [](const MemberEntry& row) -> std::string {
                return row.member_name;
            },
            [](MemberEntry& row, const char* new_name) -> bool {
                const std::string ctx = "type=" + row.type_name + " member=" + std::to_string(row.member_index);
                TypeMemberRef ref(row.type_ordinal);
                if (!ref.valid) {
                    xsql::set_vtab_error("types_members: type not found (" + ctx + ")");
                    return false;
                }
                if (row.member_index < 0 || static_cast<size_t>(row.member_index) >= ref.udt.size()) {
                    xsql::set_vtab_error("types_members: member index out of range (" + ctx + ")");
                    return false;
                }
                ref.udt[row.member_index].name = new_name ? new_name : "";
                bool ok = ref.save();
                if (ok) row.member_name = new_name ? new_name : "";
                else xsql::set_vtab_error("types_members: failed to save (" + ctx + ")");
                return ok;
            })
        .column_int64("offset", [](const MemberEntry& row) -> int64_t {
            return row.offset;
        })
        .column_int64("offset_bits", [](const MemberEntry& row) -> int64_t {
            return row.offset_bits;
        })
        .column_int64("size", [](const MemberEntry& row) -> int64_t {
            return row.size;
        })
        .column_int64("size_bits", [](const MemberEntry& row) -> int64_t {
            return row.size_bits;
        })
        .column_text_rw("member_type",
            [](const MemberEntry& row) -> std::string {
                return row.member_type;
            },
            [](MemberEntry& row, xsql::FunctionArg val) -> bool {
                const std::string ctx = "type=" + row.type_name + " member=" + std::to_string(row.member_index);
                if (val.is_nochange()) return true;
                if (val.is_null()) {
                    xsql::set_vtab_error("types_members: member_type cannot be NULL (" + ctx + ")");
                    return false;
                }

                tinfo_t member_type;
                const std::string type_text = val.as_text();
                if (!parse_member_type(type_text, member_type)) {
                    xsql::set_vtab_error("types_members: failed to parse member_type '" + type_text + "' (" + ctx + ")");
                    return false;
                }

                const asize_t member_size = member_type.get_size();
                if (member_size == BADSIZE) {
                    xsql::set_vtab_error("types_members: member_type has no fixed size '" + type_text + "' (" + ctx + ")");
                    return false;
                }

                TypeMemberRef ref(row.type_ordinal);
                if (!ref.valid) {
                    xsql::set_vtab_error("types_members: type not found (" + ctx + ")");
                    return false;
                }
                if (row.member_index < 0 || static_cast<size_t>(row.member_index) >= ref.udt.size()) {
                    xsql::set_vtab_error("types_members: member index out of range (" + ctx + ")");
                    return false;
                }
                if (ref.udt[row.member_index].size != member_size * 8) {
                    xsql::set_vtab_error(
                        "types_members: member_type cannot change member size without changing UDT layout (" + ctx + ")");
                    return false;
                }

                tinfo_t updated_tif;
                if (!updated_tif.get_named_type(get_idati(), row.type_name.c_str())) {
                    xsql::set_vtab_error("types_members: failed to load type for member_type update (" + ctx + ")");
                    return false;
                }
                if (updated_tif.set_udm_type(static_cast<size_t>(row.member_index), member_type) != TERR_OK) {
                    xsql::set_vtab_error("types_members: failed to update member_type (" + ctx + ")");
                    return false;
                }

                udt_type_data_t updated_udt;
                if (!updated_tif.get_udt_details(&updated_udt)
                    || static_cast<size_t>(row.member_index) >= updated_udt.size()) {
                    xsql::set_vtab_error("types_members: failed to refresh member_type (" + ctx + ")");
                    return false;
                }
                const udm_t& updated_member = updated_udt[row.member_index];
                row.offset = static_cast<int64_t>(updated_member.offset / 8);
                row.offset_bits = static_cast<int64_t>(updated_member.offset);
                refresh_member_type_fields(row, updated_member, get_idati());
                return true;
            })
        .column_int("is_bitfield", [](const MemberEntry& row) -> int {
            return row.is_bitfield ? 1 : 0;
        })
        .column_int("is_baseclass", [](const MemberEntry& row) -> int {
            return row.is_baseclass ? 1 : 0;
        })
        .column_text_rw("comment",
            [](const MemberEntry& row) -> std::string {
                return row.comment;
            },
            [](MemberEntry& row, const char* new_comment) -> bool {
                const std::string ctx = "type=" + row.type_name + " member=" + std::to_string(row.member_index);
                TypeMemberRef ref(row.type_ordinal);
                if (!ref.valid) {
                    xsql::set_vtab_error("types_members: type not found (" + ctx + ")");
                    return false;
                }
                if (row.member_index < 0 || static_cast<size_t>(row.member_index) >= ref.udt.size()) {
                    xsql::set_vtab_error("types_members: member index out of range (" + ctx + ")");
                    return false;
                }
                ref.udt[row.member_index].cmt = new_comment ? new_comment : "";
                bool ok = ref.save();
                if (ok) row.comment = new_comment ? new_comment : "";
                else xsql::set_vtab_error("types_members: failed to save (" + ctx + ")");
                return ok;
            })
        .column_int("mt_is_struct", [](const MemberEntry& row) -> int {
            return row.mt_is_struct ? 1 : 0;
        })
        .column_int("mt_is_union", [](const MemberEntry& row) -> int {
            return row.mt_is_union ? 1 : 0;
        })
        .column_int("mt_is_enum", [](const MemberEntry& row) -> int {
            return row.mt_is_enum ? 1 : 0;
        })
        .column_int("mt_is_ptr", [](const MemberEntry& row) -> int {
            return row.mt_is_ptr ? 1 : 0;
        })
        .column_int("mt_is_array", [](const MemberEntry& row) -> int {
            return row.mt_is_array ? 1 : 0;
        })
        .column_int("member_type_ordinal", [](const MemberEntry& row) -> int {
            return row.member_type_ordinal;
        })
        .deletable([](MemberEntry& row) -> bool {
            TypeMemberRef ref(row.type_ordinal);
            if (!ref.valid) return false;
            if (row.member_index < 0 || static_cast<size_t>(row.member_index) >= ref.udt.size()) return false;
            ref.udt.erase(ref.udt.begin() + row.member_index);
            return ref.save();
        })
        .insertable([](int argc, xsql::FunctionArg* argv) -> bool {
            if (argc < 4
                || argv[0].is_null()
                || argv[3].is_null())
                return false;

            uint32_t ordinal = static_cast<uint32_t>(argv[0].as_int());
            const char* member_name = argv[3].as_c_str();
            if (!member_name || !member_name[0]) return false;

            TypeMemberRef ref(ordinal);
            if (!ref.valid) return false;

            udm_t new_member;
            new_member.name = member_name;
            std::string type_str = "int";
            if (argc > 8 && !argv[8].is_null()) {
                const char* mt = argv[8].as_c_str();
                if (mt && mt[0]) type_str = mt;
            }
            tinfo_t member_type;
            if (!parse_member_type(type_str, member_type)) {
                xsql::set_vtab_error("types_members: failed to parse member_type '" + type_str + "'");
                return false;
            }
            const asize_t member_size = member_type.get_size();
            if (member_size == BADSIZE) {
                xsql::set_vtab_error("types_members: member_type has no fixed size '" + type_str + "'");
                return false;
            }
            new_member.type = member_type;
            new_member.size = member_size * 8;
            if (argc > 11 && !argv[11].is_null()) {
                const char* cmt = argv[11].as_c_str();
                if (cmt) new_member.cmt = cmt;
            }
            if (!ref.udt.empty()) {
                const udm_t& last = ref.udt.back();
                new_member.offset = last.offset + last.size;
            } else {
                new_member.offset = 0;
            }

            ref.udt.push_back(new_member);
            return ref.save();
        })
        .filter_eq("type_ordinal", [](int64_t ordinal) -> std::unique_ptr<xsql::RowIterator> {
            return std::make_unique<MembersInTypeIterator>(static_cast<uint32_t>(ordinal));
        }, 10.0, 5.0)
        .build();
}

// ============================================================================
// TYPES_ENUM_VALUES Table - Enum constants
// ============================================================================

} // namespace types
} // namespace idasql
