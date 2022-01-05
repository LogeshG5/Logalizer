#include "translator.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include "path_variable_utils.h"

using namespace Logalizer::Config;

std::string Translator::fetch_values_regex(std::string const &line, std::vector<variable> const &variables)
{
   std::string value;
   if (variables.size() == 0) return value;

   value = "(";

   for (auto const &v : variables) {
      std::regex regex(v.startswith + "(.*)" + v.endswith);
      std::smatch match;
      regex_search(line, match, regex);
      if (match.size() == 2) {
         value += match[1].str() + ", ";
      }
   }

   if (value.back() == ' ') {
      value.erase(value.end() - 2, value.end());
   }

   value += ")";
   return value;
}

std::string Translator::fetch_values_braced(std::string const &line, std::vector<variable> const &variables)
{
   std::string value;
   if (variables.size() == 0) return value;

   value = "(";

   for (auto const &v : variables) {
      auto start_point = line.find(v.startswith);
      if (start_point == std::string::npos) continue;

      start_point += v.startswith.size();
      auto end_point = line.find(v.endswith, start_point);
      if (end_point == std::string::npos) continue;

      std::string capture(line.cbegin() + static_cast<long>(start_point), line.cbegin() + static_cast<long>(end_point));

      value += capture + ", ";
   }

   if (value.back() == ' ') {
      value.erase(value.end() - 2, value.end());
   }

   value += ")";
   return value;
}

std::string Translator::capture_values(variable const &var, std::string const &content)
{
   auto start_point = content.find(var.startswith);
   if (start_point == std::string::npos) return " ";

   start_point += var.startswith.size();
   const auto end_point = content.find(var.endswith, start_point);
   if (end_point == std::string::npos) return " ";

   std::string capture(content.cbegin() + static_cast<long>(start_point),
                       content.cbegin() + static_cast<long>(end_point));

   return capture;
}

std::vector<std::string> Translator::fetch_values(std::string const &line, std::vector<variable> const &variables)
{
   std::vector<std::string> value;
   if (variables.size() == 0) return {};

   std::transform(cbegin(variables), cend(variables), std::back_inserter(value),
                  [&line, this](auto const &var) { return capture_values(var, line); });

   return value;
}

std::string Translator::pack_parameters(std::vector<std::string> const &v)
{
   auto comma_fold = [](std::string a, std::string b) { return a + ", " + b; };
   const std::string initial_value = "(" + v[0];
   std::string params = std::accumulate(next(cbegin(v)), cend(v), initial_value, comma_fold) + ")";
   return params;
}

std::string Translator::fill_values_formatted(std::vector<std::string> const &values, std::string const &line_to_fill)
{
   std::string filled_line = line_to_fill;
   for (size_t i = 1, len = values.size(); i <= len; ++i) {
      const std::string token = "${" + std::to_string(i) + "}";
      Utils::replace_all(&filled_line, token, values[i - 1]);
   }
   return filled_line;
}

std::string Translator::fill_values(std::vector<std::string> const &values, std::string const &line_to_fill)
{
   std::string filled_line;
   const bool formatted_print = (line_to_fill.find("${1}") != std::string::npos);
   if (formatted_print) {
      filled_line = fill_values_formatted(values, line_to_fill);
   }
   else if (values.size()) {
      filled_line = line_to_fill + pack_parameters(values);
   }
   else {
      filled_line = line_to_fill;
   }
   return filled_line;
}

[[nodiscard]] bool Translator::is_blacklisted(std::string const &line, std::vector<std::string> const &blacklists)
{
   return std::any_of(cbegin(blacklists), cend(blacklists),
                      [&line](auto const &bl) { return line.find(bl) != std::string::npos; });
}

auto Translator::match(std::string const &line, std::vector<translation> const &translations,
                       std::vector<std::string> const &blacklists)
{
   auto found = std::find_if(cbegin(translations), cend(translations), [&line](auto const &tr) { return tr.in(line); });
   if (found != cend(translations)) {
      if (!is_blacklisted(line, blacklists)) {
         return found;
      }
   }

   return found;
}

[[nodiscard]] bool Translator::is_deleted(std::string const &line, std::vector<std::string> const &delete_lines,
                                          std::vector<std::regex> const &delete_lines_regex) noexcept
{
   bool deleted = std::any_of(cbegin(delete_lines), cend(delete_lines),
                              [&line](auto const &dl) { return line.find(dl) != std::string::npos; });

   if (deleted) return true;

   deleted = std::any_of(cbegin(delete_lines_regex), cend(delete_lines_regex),
                         [&line](auto const &dl) { return regex_search(line, dl); });

   return deleted;
}

void Translator::replace(std::string *line, std::vector<replacement> const &replacemnets)
{
   std::for_each(cbegin(replacemnets), cend(replacemnets),
                 [&](auto const &entry) { Utils::replace_all(line, entry.search, entry.replace); });
}

void Translator::add_translation(std::vector<std::string> &translations, std::string &&translation,
                                 const Logalizer::Config::translation trans_cfg,
                                 std::unordered_map<size_t, size_t> &trans_count)
{
   const bool repeat_allowed = trans_cfg.repeat;
   bool add_translation = true;
   if (trans_cfg.count == count_type::scoped) {
      if (translations.empty()) {
         trans_count[0]++;
      }
      if (translation != translations.back()) {
         trans_count[translations.size()]++;
      }
      else {
         add_translation = false;
         trans_count[translations.size() - 1]++;
      }
   }
   else if (trans_cfg.count == count_type::global) {
      auto first = std::find(cbegin(translations), cend(translations), translation);
      if (first != end(translations)) {
         trans_count[static_cast<size_t>(std::distance(cbegin(translations), first))]++;
         add_translation = false;
      }
   }
   if ((add_translation && repeat_allowed) ||
       std::none_of(cbegin(translations), cend(translations),
                    [&translation](auto const &entry) { return entry == translation; })) {
      add_translation = true;
   }
   if (add_translation) {
      translations.emplace_back(std::move(translation));
   }
}

void Translator::update_count(std::vector<std::string> &translations,
                              std::unordered_map<size_t, size_t> const &trans_count)
{
   for (auto const &[index, count] : trans_count) {
      std::cout << index << ": " << count << "\n";
      Utils::replace_all(&translations[index], "${count}", std::to_string(count));
   }
}

void Translator::translate_file(std::string const &trace_file_name, std::string const &translation_file_name,
                                ConfigParser const &config)
{
   std::ifstream trace_file(trace_file_name);
   const std::string trim_file_name = trace_file_name + ".trim.log";
   std::ofstream trimmed_file(trim_file_name);
   std::vector<std::string> translations;
   std::unordered_map<size_t, size_t> trans_count;

   const auto pre_text = config.get_wrap_text_pre();
   std::copy(pre_text.cbegin(), pre_text.cend(), std::back_inserter(translations));

   for (std::string line; getline(trace_file, line);) {
      if (is_deleted(line, config.get_delete_lines(), config.get_delete_lines_regex())) {
         continue;
      }

      replace(&line, config.get_replace_words());
      trimmed_file << line << '\n';

      const auto translation_cfg = match(line, config.get_translations(), config.get_blacklists());
      if (translation_cfg != cend(config.get_translations())) {
         std::vector<std::string> values = fetch_values(line, translation_cfg->variables);
         std::string translation = fill_values(values, translation_cfg->print);
         add_translation(translations, std::move(translation), (*translation_cfg), trans_count);
      }
   }
   update_count(translations, trans_count);
   const auto post_text = config.get_wrap_text_post();
   std::copy(post_text.cbegin(), post_text.cend(), std::back_inserter(translations));

   Utils::mkdir(Utils::dir_file(translation_file_name).first);

   std::ofstream translation_file(translation_file_name);
   if (config.get_auto_new_line()) {
      std::copy(translations.cbegin(), translations.cend(), std::ostream_iterator<std::string>(translation_file, "\n"));
   }
   else {
      std::copy(translations.cbegin(), translations.cend(), std::ostream_iterator<std::string>(translation_file));
   }
   translation_file.close();

   trimmed_file.close();
   trace_file.close();
   remove(trace_file_name.c_str());
   rename(trim_file_name.c_str(), trace_file_name.c_str());
}