/* Copyright (c) 2023 Percona LLC and/or its affiliates. All rights reserved.

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

#ifndef MASKING_FUNCTIONS_SQL_CONTEXT_HPP
#define MASKING_FUNCTIONS_SQL_CONTEXT_HPP

#include "masking_functions/sql_context_fwd.hpp"  // IWYU pragma: export

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "masking_functions/command_service_tuple_fwd.hpp"

namespace masking_functions {

// A wrapper class that uses MySQL connection services under the hood and
// simplifies query execution operations.
// It requires an instance of the 'command_service_tuple' class for
// construction.
class sql_context {
 public:
  template <std::size_t NumberOfFields>
  using field_value_container = std::array<std::string_view, NumberOfFields>;

  template <std::size_t NumberOfFields>
  using row_callback =
      std::function<void(const field_value_container<NumberOfFields> &)>;

  sql_context(const command_service_tuple &services,
              sql_context_registry_access initialization_registry_locking_mode,
              sql_context_registry_access operation_registry_locking_mode,
              bool initialize_thread);

  sql_context(sql_context const &) = delete;
  sql_context(sql_context &&) noexcept = default;

  sql_context &operator=(sql_context const &) = delete;
  sql_context &operator=(sql_context &&) noexcept = default;

  ~sql_context() = default;

  void reset();

  const command_service_tuple &get_services() const noexcept {
    return *impl_.get_deleter().services;
  }

  template <std::size_t NumberOfFields>
  void execute_select(std::string_view query,
                      const row_callback<NumberOfFields> &callback) {
    execute_select_internal(
        query, NumberOfFields,
        [&callback](char **field_values, std::size_t *lengths) {
          field_value_container<NumberOfFields> wrapped_field_values;
          std::transform(field_values, field_values + NumberOfFields, lengths,
                         std::begin(wrapped_field_values),
                         [](char *str, std::size_t len) {
                           return std::string_view{str, len};
                         });
          callback(wrapped_field_values);
        });
  }

  bool execute_dml(std::string_view query);

 private:
  struct deleter {
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    const command_service_tuple *services;

    void operator()(void *ptr) const noexcept;
  };
  using impl_type = std::unique_ptr<void, deleter>;
  impl_type impl_;

  using row_internal_callback = std::function<void(char **, std::size_t *)>;
  void execute_select_internal(std::string_view query,
                               std::size_t number_of_fields,
                               const row_internal_callback &callback);
  [[noreturn]] void raise_with_error_message(std::string_view prefix);
};

}  // namespace masking_functions

#endif  // MASKING_FUNCTIONS_SQL_CONTEXT_HPP
