#include <sys/stat.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string_view>
#include "LogalizerConfig.h"
#include "jsonconfigparser.h"
#include "spdlog/spdlog.h"
#include "translator.h"

using std::chrono::high_resolution_clock;

namespace fs = std::filesystem;

void printHelp()
{
   std::cout << "\n"
                "Logalizer v"
             << LOGALIZER_VERSION_MAJOR << "." << LOGALIZER_VERSION_MINOR << "\n"
             << "  Helper to visualize and understand logs.\n"
                "  Logesh Gopalakrishnan\n\n"
                "Usage:\n"
                "  logalizer -c <config> -f <log>\n"
                "  logalizer -f <log>\n"
                "  logalizer -h | --help\n"
                "  logalizer --version\n"
                "\n"
                "Options:\n"
                "  -h --help        Show this screen\n"
                "  --config-help    Show sample configuration\n"
                "  --version        Show version\n"
                "  -c <config>      Translation configuration file. Defaults to config.json\n"
                "  -f <log>         Log file to be interpreted\n"
                "\n"
                "Example:\n"
                "  logalizer -c config.json -f trace.log\n"
                "  logalizer -f trace.log\n"
             << std::endl;
}

using namespace Logalizer::Config;

/**
 * @brief To store values from command line arguments
 *
 */
struct CMD_Args {
   /**
    * @brief Path to input config file
    *
    */
   const std::string config_file;
   /**
    * @brief Input file that needs to be translated
    *
    */
   const std::string log_file;
};

CMD_Args parse_cmd_line(const std::vector<std::string_view>& args)
{
   std::string log_file;
   std::string config_file;
   for (auto it = cbegin(args), endit = cend(args); it != endit; ++it) {
      if ((*it == "-f" || *it == "--file") && next(it) != endit) {
         log_file = *(next(it));
      }
      else if ((*it == "-c" || *it == "--config") && next(it) != endit) {
         config_file = *(next(it));
      }
      else if (*it == "-h" || *it == "--help") {
         printHelp();
         exit(0);
      }
      else if (*it == "--version") {
         std::cout << "Logalizer v" << LOGALIZER_VERSION_MAJOR << "." << LOGALIZER_VERSION_MINOR << std::endl;
         exit(0);
      }
   }
   if (log_file.empty()) {
      printHelp();
      exit(0);
   }

   if (config_file.empty()) {
      fs::path exe_path = fs::path(args.at(0)).remove_filename();
      config_file = (exe_path / "config.json").string();
   }

   if (!fs::exists(config_file)) {
      std::cerr << config_file << " : config file not available\n\n";
      printHelp();
      exit(1);
   }
   if (!fs::exists(log_file)) {
      std::cerr << log_file << " : not available\n";
      exit(1);
   }

   return {config_file, log_file};
}

void backup_if_not_exists(const std::string& original, const std::string& backup)
{
   if (original.empty() || backup.empty()) {
      return;
   }

   try {
      fs::create_directories(fs::path(backup).remove_filename());
      if (!fs::exists(backup)) {
         fs::copy_file(original, backup);
      }
   }
   catch (std::exception& e) {
      std::cerr << "Backup failed : " << e.what();
   }
}

path_vars get_path_vars(const std::string& log_file)
{
   fs::path log_file_path = log_file;
   path_vars path_details;
   path_details.dir = log_file_path.parent_path().string();
   path_details.file = log_file_path.filename().string();
   path_details.file_no_ext = fs::path(path_details.file).replace_extension("").string();
   return path_details;
}

static std::chrono::time_point<high_resolution_clock> start_time;
static std::chrono::time_point<high_resolution_clock> end_time;

void start_benchmark()
{
   start_time = std::chrono::high_resolution_clock::now();
}

void end_benchmark(std::string const& print)
{
   end_time = std::chrono::high_resolution_clock::now();
   auto count = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
   std::cout << '[' << count << "ms] " << print << '\n';
}

int main(int argc, char** argv)
{
   const std::vector<std::string_view> args(argv, argv + argc);
   const CMD_Args cmd_args = parse_cmd_line(args);

   start_benchmark();
   JsonConfigParser config(cmd_args.config_file);
   try {
      config.set_path_variables(get_path_vars(cmd_args.log_file));
      config.read_config_file();
      config.load_configurations();
   }
   catch (std::exception& e) {
      std::cerr << "Loading configuration failed\n";
      std::cerr << e.what();
      exit(2);
   }
   end_benchmark("Configuration loaded");

   backup_if_not_exists(cmd_args.log_file, config.get_backup_file());

   Translator translator(config);
   start_benchmark();
   translator.translate_file(cmd_args.log_file);
   end_benchmark("Translation file generated");

   start_benchmark();
   translator.execute_commands();
   end_benchmark("Executed");
   return 0;
}
