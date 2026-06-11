// Copyright (c) 2024-2026 Elias Bachaalany
// SPDX-License-Identifier: MPL-2.0
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * entities_imports.hpp - `imports` table (imported symbols + module).
 */

#pragma once

#include "core_common.hpp"

namespace idasql {
namespace symbols {

struct ImportInfo {
  int module_idx;
  ea_t ea;
  std::string name;
  uval_t ord;
  std::string folder_path;
  std::string full_path;
};

struct ImportEnumContext {
  std::vector<ImportInfo> *cache;
  const std::unordered_map<uint64_t, dirtrees::DirtreePathInfo> *folder_paths;
  int module_idx;
};

std::string get_import_module_name_safe(int idx);

CachedTableDef<ImportInfo> define_imports();

} // namespace symbols
} // namespace idasql
