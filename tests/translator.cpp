#include "translator.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "catch.hpp"
#include "configparser_mock.h"

namespace fs = std::filesystem;
using namespace Logalizer::Config;
using namespace unit_test;

TEST_CASE("is_blacklisted")
{
   ConfigParserMock config;
   std::vector<std::string> blacklist = {"b1", "b2"};
   config.set_blacklists(blacklist);

   TranslatorTesterProxy tr(Translator{config});
   std::string line;

   line = "this is a text";
   CHECK_FALSE(tr.is_blacklisted(line));

   line = "this is a text with b1";
   CHECK(tr.is_blacklisted(line));

   line = "this is a text with b2";
   CHECK(tr.is_blacklisted(line));
}

TEST_CASE("is_deleted")
{
   ConfigParserMock config;
   std::vector<std::string> deleted = {"d1", "d2"};
   std::vector<std::regex> deleted_regex = {std::regex(
       " _r.*x_ ", std::regex_constants::grep | std::regex_constants::nosubs | std::regex_constants::optimize)};
   config.set_delete_lines(deleted);
   config.set_delete_lines_regex(deleted_regex);

   TranslatorTesterProxy tr(Translator{config});
   std::string line;

   line = "this is a text for _regex_ testing";
   CHECK(tr.is_deleted(line));

   line = "this is some text with d1";
   CHECK(tr.is_deleted(line));

   line = "this is some text with d2";
   CHECK(tr.is_deleted(line));

   line = "this is some text";
   CHECK_FALSE(tr.is_deleted(line));
}

TEST_CASE("replace")
{
   ConfigParserMock config;
   std::vector<replacement> replacements = {{"s1", "r1"}, {"s2", "r2"}};
   config.set_replace_words(replacements);

   TranslatorTesterProxy tr(Translator{config});
   std::string line;
   line = "Text with s1 S1 s2 S2";
   std::string expected = "Text with r1 S1 r2 S2";
   tr.replace(&line);
   CHECK(line == expected);
}

TEST_CASE("translate basic patterns and print with manual variable capture")
{
   std::string tr_file = (fs::temp_directory_path() / "tr.txt").string();
   std::string in_file = (fs::temp_directory_path() / "input.log").string();
   std::ofstream file(in_file);
   ConfigParserMock config;
   config.set_translation_file(tr_file);
   std::vector<translation> translations;
   Translator tor(config);
   std::ifstream read_file(tr_file);
   std::string read_line;
   std::vector<std::string> lines;

   SECTION("Single pattern and print")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor"};
      tr.print = "TemperatureChanged";
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "TemperatureChanged");
   }

   SECTION("Multiple patterns and print")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "TemperatureChanged";
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "TemperatureChanged");
   }

   SECTION("Auto variable capture")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature";
      tr.variables = {{"= ", "C"}};
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "Temperature(45)");
   }

   SECTION("Auto variable capture with empty end point that captures till the end")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45Celcius";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature";
      tr.variables = {{"= ", ""}};
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "Temperature(45Celcius)");
   }

   SECTION("Auto variable capture with no matching end point that captures till the end")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45Celcius";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature";
      tr.variables = {{"= ", "Farenheit"}};
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "Temperature(45Celcius)");
   }

   SECTION("Manual variable capture")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature is ${1} degrees";
      tr.variables = {{"= ", "C"}};
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "Temperature is 45 degrees");
   }

   SECTION("Auto multi variable capture")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C, state = HighTemp;";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature";
      tr.variables = {{"= ", "C"}, {"state = ", ";"}};
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "Temperature(45, HighTemp)");
   }

   SECTION("Auto multi variable capture out of order variables")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C, state = HighTemp;";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature";
      tr.variables = {{"state = ", ";"}, {"= ", "C"}};
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "Temperature(HighTemp, 45)");
   }

   SECTION("Manual multi variable capture out of order variables")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C, state = HighTemp;";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature(${2} - ${1})";
      tr.variables = {{"state = ", ";"}, {"= ", "C"}};
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      getline(read_file, read_line);
      CHECK(read_line == "Temperature(45 - HighTemp)");
   }

   SECTION("duplicates: default(allowed)")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "TemperatureChanged";
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      for (std::string read_line; getline(read_file, read_line);) {
         lines.push_back(read_line);
      }
      CHECK(lines.size() == 2);
      CHECK(lines.at(0) == "TemperatureChanged");
      CHECK(lines.at(1) == "TemperatureChanged");
   }

   SECTION("duplicates: allowed")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "TemperatureChanged";
      tr.duplicates = duplicates_t::allowed;
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      for (std::string read_line; getline(read_file, read_line);) {
         lines.push_back(read_line);
      }
      CHECK(lines.size() == 2);
      CHECK(lines.at(0) == "TemperatureChanged");
      CHECK(lines.at(1) == "TemperatureChanged");
   }

   SECTION("duplicates: remove")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "TemperatureChanged";
      tr.duplicates = duplicates_t::remove;
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      for (std::string read_line; getline(read_file, read_line);) {
         lines.push_back(read_line);
      }
      CHECK(lines.size() == 1);
      CHECK(lines.at(0) == "TemperatureChanged");
   }

   SECTION("duplicates: remove_continuous")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "TemperatureChanged";
      tr.duplicates = duplicates_t::remove_continuous;
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      for (std::string read_line; getline(read_file, read_line);) {
         lines.push_back(read_line);
      }
      CHECK(lines.size() == 1);
      CHECK(lines.at(0) == "TemperatureChanged");
   }

   SECTION("duplicates: remove_continuous not continuous")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature";
      tr.variables = {{"= ", "C"}};
      tr.duplicates = duplicates_t::remove_continuous;
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      for (std::string read_line; getline(read_file, read_line);) {
         lines.push_back(read_line);
      }
      CHECK(lines.size() == 2);
      CHECK(lines.at(0) == "Temperature(45)");
      CHECK(lines.at(1) == "Temperature(49)");
   }

   SECTION("duplicates: remove_continuous not continuous")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file << "[INFO]: TemperatureSensor: temperature = 46C\n";
      file << "[INFO]: TemperatureSensor: temperature = 46C\n";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "Temperature";
      tr.variables = {{"= ", "C"}};
      tr.duplicates = duplicates_t::remove_continuous;
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      for (std::string read_line; getline(read_file, read_line);) {
         lines.push_back(read_line);
      }
      CHECK(lines.size() == 3);
      CHECK(lines.at(0) == "Temperature(45)");
      CHECK(lines.at(1) == "Temperature(49)");
      CHECK(lines.at(2) == "Temperature(46)");
   }

   SECTION("duplicates: count")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "TemperatureChanged ${count} times";
      tr.duplicates = duplicates_t::count;
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      for (std::string read_line; getline(read_file, read_line);) {
         lines.push_back(read_line);
      }
      CHECK(lines.size() == 1);
      CHECK(lines.at(0) == "TemperatureChanged 3 times");
   }

   SECTION("duplicates: count_continuous")
   {
      file << "[INFO]: TemperatureSensor: temperature = 45C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file << "[INFO]: TemperatureSensor: temperature = 49C\n";
      file.close();
      translation tr;
      tr.patterns = {"TemperatureSensor", "temperature"};
      tr.print = "TemperatureChanged to ${1} ${count} times";
      tr.variables = {{"= ", "C"}};
      tr.duplicates = duplicates_t::count;
      translations.push_back(tr);
      config.set_translations(translations);
      tor.translate_file(in_file);
      for (std::string read_line; getline(read_file, read_line);) {
         lines.push_back(read_line);
      }
      CHECK(lines.size() == 2);
      CHECK(lines.at(0) == "TemperatureChanged to 45 1 times");
      CHECK(lines.at(1) == "TemperatureChanged to 49 2 times");
   }
}
