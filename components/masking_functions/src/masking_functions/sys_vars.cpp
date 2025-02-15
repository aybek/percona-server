/* Copyright (c) 2024 Percona LLC and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#include "masking_functions/sys_vars.hpp"

#include <climits>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

#include <my_inttypes.h>

// #include <mysql/components/component_implementation.h>
#include <mysql/components/services/bits/system_variables_bits.h>
#include <mysql/components/services/component_sys_var_service.h>

#include "masking_functions/component_sys_variable_service_tuple.hpp"
#include "masking_functions/primitive_singleton.hpp"

namespace {

using global_component_sys_variable_services =
    masking_functions::primitive_singleton<
        masking_functions::component_sys_variable_service_tuple>;

constexpr const char component_name[]{"masking_functions"};
constexpr const char masking_database_var_name[]{"masking_database"};
constexpr const char flush_interval_var_name[]{
    "dictionaries_flush_interval_seconds"};

char default_database_name[]{"mysql"};
const ulonglong default_flush_interval_seconds = 0;

bool is_database_name_initialised = false;
bool is_flush_interval_initialised = false;

char *database_name;
ulonglong flush_interval_seconds = 0;

}  // anonymous namespace

namespace masking_functions {

std::string_view get_dict_database_name() noexcept { return database_name; }

std::uint64_t get_flush_interval_seconds() noexcept {
  return flush_interval_seconds;
}

bool register_sys_vars() {
  STR_CHECK_ARG(str) check_db_name{default_database_name};

  const auto &services{global_component_sys_variable_services::instance()};
  // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
  void *casted_database_name{static_cast<void *>(&database_name)};
  if (services.registrator->register_variable(
          component_name, masking_database_var_name,
          PLUGIN_VAR_STR | PLUGIN_VAR_MEMALLOC | PLUGIN_VAR_RQCMDARG |
              PLUGIN_VAR_READONLY,
          "Specifies the database to use for data masking dictionaries "
          "at server startup.",
          nullptr, nullptr, static_cast<void *>(&check_db_name),
          casted_database_name) != 0) {
    return false;
  }
  is_database_name_initialised = true;

  INTEGRAL_CHECK_ARG(ulonglong)
  check_flush_interval{default_flush_interval_seconds, 0, ULLONG_MAX, 1};

  if (services.registrator->register_variable(
          component_name, flush_interval_var_name,
          PLUGIN_VAR_LONGLONG | PLUGIN_VAR_UNSIGNED | PLUGIN_VAR_RQCMDARG |
              PLUGIN_VAR_READONLY,
          "Sets the interval, in seconds, to wait before attempting to "
          "schedule another flush of the data masking dictionaries table to "
          "the memory data masking dictionaries cache following a restart or "
          "previous execution.",
          nullptr, nullptr, static_cast<void *>(&check_flush_interval),
          static_cast<void *>(&flush_interval_seconds)) != 0) {
    return false;
  }
  is_flush_interval_initialised = true;

  return true;
}

bool unregister_sys_vars() {
  bool is_success = true;

  const auto &services{global_component_sys_variable_services::instance()};
  if (is_database_name_initialised &&
      services.unregistrator->unregister_variable(
          component_name, masking_database_var_name) != 0) {
    is_success = false;
  }

  if (is_flush_interval_initialised &&
      services.unregistrator->unregister_variable(
          component_name, flush_interval_var_name) != 0) {
    is_success = false;
  }

  return is_success;
}

bool check_sys_vars(std::string &error_message) {
  if (database_name == nullptr || std::strlen(database_name) == 0) {
    error_message = "Bad masking_functions.masking_database value";
    return false;
  }

  return true;
}

}  // namespace masking_functions
