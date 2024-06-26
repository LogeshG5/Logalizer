#pragma once

#include <nlohmann/json.hpp>
#include "configparser.h"

namespace Logalizer::Config {
/**
 * @brief Parses Json configuration
 *
 */
using json = nlohmann::ordered_json;

class JsonConfigParser final : public ConfigParser {
  public:
   explicit JsonConfigParser(std::string config_file = "config.json");
   explicit JsonConfigParser(json config);
   void load_disabled_categories() override;
   void load_translations() override;
   void load_wrap_text() override;
   void load_pairs() override;
   void load_blacklists() override;
   void load_delete_lines() override;
   void load_replace_words() override;
   void load_execute() override;
   void load_translation_file() override;
   void load_backup_file() override;
   void load_auto_new_line() override;

   void read_config_file() override;

  private:
   json config_;
   std::string config_file_;
   template <class T>
   T get_value_or(json const& config, std::string const& name, T value);
   std::vector<variable> get_variables(json const& config);
   std::vector<translation> load_translations(json const& config, std::string const& name);
   std::vector<translation> load_translations_csv(std::string const& translations_csv_file);
};
}  // namespace Logalizer::Config
