#pragma once

#include <regex>
#include <utility>
#include "config_types.h"

namespace Logalizer::Config {

static const std::string TAG_TRANSLATIONS = "translations";
static const std::string TAG_TRANSLATIONS_CSV = "translations_csv";
static const std::string TAG_WRAPTEXT_PRE = "wrap_text_pre";
static const std::string TAG_WRAPTEXT_POST = "wrap_text_post";
static const std::string TAG_DISABLE_CATEGORY = "disable_group";
static const std::string TAG_BLACKLIST = "blacklist";
static const std::string TAG_DELETE_LINES = "delete_lines";
static const std::string TAG_REPLACE_WORDS = "replace_words";
static const std::string TAG_EXECUTE = "execute";
static const std::string TAG_TRANSLATION_FILE = "translation_file";
static const std::string TAG_BACKUP_FILE = "backup_file";
static const std::string TAG_AUTO_NEW_LINE = "auto_new_line";
static const std::string TAG_CATEGORY = "group";
static const std::string TAG_PATTERNS = "patterns";
static const std::string TAG_PRINT = "print";
static const std::string TAG_VARIABLES = "variables";
static const std::string TAG_STARTS_WITH = "startswith";
static const std::string TAG_ENDS_WITH = "endswith";
static const std::string TAG_DUPLICATES = "duplicates";
static const std::string TAG_ENABLE = "enable";
static const std::string TAG_DUPLICATES_ALLOWED = "allowed";
static const std::string TAG_DUPLICATES_REMOVE = "remove";
static const std::string TAG_DUPLICATES_REMOVE_CONTINUOUS = "remove_continuous";
static const std::string TAG_DUPLICATES_COUNT = "count";
static const std::string TAG_DUPLICATES_COUNT_CONTINUOUS = "count_continuous";
static const std::string TAG_PAIRS = "pairs";
static const std::string TAG_PAIRSOURCE = "source";
static const std::string TAG_PAIRSWITH = "pairswith";
static const std::string TAG_PAIRBEFORE = "before";
static const std::string TAG_PAIRERROR = "error";

static const std::string VAR_FILE_DIR_NAME = R"(\$\{fileDirname\})";
static const std::string VAR_EXE_DIR_NAME = R"(\$\{exeDirname\})";
static const std::string VAR_FILE_BASE_NO_EXTENSION = R"(\$\{fileBasenameNoExtension\})";
static const std::string VAR_FILE_BASE_WITH_EXTENSION = R"(\$\{fileBasename\})";
/**
 * @brief Base class that defines what configurations are needed for Logalizer
 *
 *
 */
class ConfigParser {
  public:
   ConfigParser() = default;
   virtual ~ConfigParser() = default;
   ConfigParser(ConfigParser&&) = default;
   ConfigParser(const ConfigParser&) = default;
   ConfigParser& operator=(const ConfigParser&) = default;
   ConfigParser& operator=(ConfigParser&&) = default;

   virtual void load_configurations() final;
   virtual void read_config_file() = 0;
   virtual bool is_disabled(const std::string& category) final;
   virtual duplicates_t get_duplicate_type(std::string const& dup) final;

   void set_path_variables(path_vars input_file_details)
   {
      input_file_details_ = std::move(input_file_details);
   }

   [[nodiscard]] inline std::vector<translation> const& get_translations() const noexcept
   {
      return translations_;
   }

   [[nodiscard]] inline std::vector<pair> const& get_pairs() const noexcept
   {
      return pairs_;
   }

   [[nodiscard]] inline std::vector<std::string> const& get_disabled_categories() const noexcept
   {
      return disabled_categories_;
   }

   [[nodiscard]] inline std::vector<std::string> const& get_wrap_text_pre() const noexcept
   {
      return wrap_text_pre_;
   }

   [[nodiscard]] inline std::vector<std::string> const& get_wrap_text_post() const noexcept
   {
      return wrap_text_post_;
   }

   [[nodiscard]] inline std::vector<std::regex> const& get_delete_lines_regex() const noexcept
   {
      return delete_lines_regex_;
   }
   [[nodiscard]] inline std::vector<std::string> const& get_delete_lines() const noexcept
   {
      return delete_lines_;
   }
   [[nodiscard]] inline std::vector<replacement> const& get_replace_words() const noexcept
   {
      return replace_words_;
   }
   [[nodiscard]] inline std::vector<std::string> const& get_blacklists() const noexcept
   {
      return blacklists_;
   }
   [[nodiscard]] inline std::vector<std::string> const& get_execute_commands() const noexcept
   {
      return execute_commands_;
   }
   [[nodiscard]] inline std::string const& get_translation_file() const noexcept
   {
      return translation_file_;
   }

   [[nodiscard]] inline std::string const& get_backup_file() const noexcept
   {
      return backup_file_;
   }

   [[nodiscard]] inline bool const& get_auto_new_line() const noexcept
   {
      return auto_new_line_;
   }

  protected:
   void set_translations(std::vector<translation> translations)
   {
      translations_ = std::move(translations);
   }

   void set_pairs(std::vector<pair> pairs)
   {
      pairs_ = std::move(pairs);
   }

   void set_disabled_categories(std::vector<std::string> disabled_categories)
   {
      disabled_categories_ = std::move(disabled_categories);
   }

   void set_wrap_text_pre(std::vector<std::string> wrap_text_pre)
   {
      wrap_text_pre_ = std::move(wrap_text_pre);
   }

   void set_wrap_text_post(std::vector<std::string> wrap_text_post)
   {
      wrap_text_post_ = std::move(wrap_text_post);
   }

   void set_delete_lines_regex(std::vector<std::regex> delete_lines_regex)
   {
      delete_lines_regex_ = std::move(delete_lines_regex);
   }

   void set_delete_lines(std::vector<std::string> delete_lines)
   {
      delete_lines_ = std::move(delete_lines);
   }

   void set_replace_words(std::vector<replacement> replace_words)
   {
      replace_words_ = std::move(replace_words);
   }

   void set_blacklists(std::vector<std::string> blacklists)
   {
      blacklists_ = std::move(blacklists);
   }

   void set_execute_commands(std::vector<std::string> execute_commands)
   {
      execute_commands_ = std::move(execute_commands);
   }

   void set_translation_file(std::string translation_file)
   {
      translation_file_ = std::move(translation_file);
   }

   void set_backup_file(std::string backup_file)
   {
      backup_file_ = std::move(backup_file);
   }

   void set_auto_new_line_(bool auto_new_line)
   {
      auto_new_line_ = auto_new_line;
   }

   void set_auto_new_line(bool auto_new_line)
   {
      auto_new_line_ = auto_new_line;
   }

  private:
   virtual void update_path_variables() final;
   virtual void load_disabled_categories() = 0;
   virtual void load_translations() = 0;
   virtual void load_wrap_text() = 0;
   virtual void load_blacklists() = 0;
   virtual void load_delete_lines() = 0;
   virtual void load_replace_words() = 0;
   virtual void load_execute() = 0;
   virtual void load_translation_file() = 0;
   virtual void load_backup_file() = 0;
   virtual void load_auto_new_line() = 0;
   virtual void load_pairs() = 0;

   std::vector<translation> translations_;
   std::vector<pair> pairs_;
   std::vector<std::string> disabled_categories_;
   std::vector<std::string> wrap_text_pre_;
   std::vector<std::string> wrap_text_post_;
   std::vector<std::regex> delete_lines_regex_;
   std::vector<std::string> delete_lines_;
   std::vector<replacement> replace_words_;
   std::vector<std::string> blacklists_;
   std::vector<std::string> execute_commands_;
   std::string translation_file_;
   std::string backup_file_;
   bool auto_new_line_ = true;
   path_vars input_file_details_;
};
}  // namespace Logalizer::Config
