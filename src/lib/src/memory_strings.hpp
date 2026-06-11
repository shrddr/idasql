// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_strings.hpp - `strings` table and string-literal helpers.
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace memory {

// String helpers
int get_string_width(int strtype);
const char *get_string_width_name(int strtype);
const char *get_string_type_name(int strtype);
int get_string_layout(int strtype);
const char *get_string_layout_name(int strtype);
int get_string_encoding(int strtype);
std::string get_string_content(const string_info_t &si);

CachedTableDef<string_info_t> define_strings();

} // namespace memory
} // namespace idasql
