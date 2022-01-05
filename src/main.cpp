//
//  https://github.com/edmBernard/include-graph
//
//  Created by Erwan BERNARD on 11/09/2021.
//
//  Copyright (c) 2021 Erwan BERNARD. All rights reserved.
//  Distributed under the Apache License, Version 2.0. (See accompanying
//  file LICENSE or copy at http://www.apache.org/licenses/LICENSE-2.0)
//

#include <geometry.hpp>
#include <save.hpp>

#include <cxxopts.hpp>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <regex>
#include <set>

namespace fs = std::filesystem;

bool isValidExtension(const std::string& ext) {
  std::vector<std::string> validExtention = {".cpp", ".hpp"};
  for (auto v : validExtention) {
    if (v == ext) {
      return true;
    }
  }
  return false;
}

int main(int argc, char *argv[]) try {

  spdlog::cfg::load_env_levels();

  // =================================================================================================
  // CLI
  cxxopts::Options options(argv[0], "Description");
  options.positional_help("output [level]").show_positional_help();

  // clang-format off
  options.add_options()
    ("h,help", "Print help")
    ("sources", "Source folder", cxxopts::value<std::string>()->default_value("."))
    ("o,output", "Output filename (.svg)", cxxopts::value<std::string>())
    ;
  // clang-format on
  options.parse_positional({"output"});
  auto clo = options.parse(argc, argv);

  if (clo.count("help")) {
    fmt::print("{}", options.help());
    return EXIT_SUCCESS;
  }

  if (!clo.count("output")) {
    spdlog::error("Output filename is required");
    return EXIT_FAILURE;
  }

  const std::string filename = clo["output"].as<std::string>();

  const std::filesystem::path inputFolder = clo["sources"].as<std::string>();
  if (!(std::filesystem::exists(inputFolder) && std::filesystem::is_directory(inputFolder))) {
    spdlog::error("Source folder missing or not a directory");
    return EXIT_FAILURE;
  }

  // =================================================================================================
  // Code
  auto start_temp = std::chrono::high_resolution_clock::now();

  // =================================================================================================
  // Parsing
  std::unordered_multimap<std::string, std::string> dependencyGraph;
  std::set<std::string> uniqueHeader;

  for (auto& p : fs::recursive_directory_iterator(inputFolder)) {

    const fs::path filename = p.path();
    if (!fs::is_regular_file(filename)) {
      continue;
    }
    if (!isValidExtension(filename.extension().string())) {
      continue;
    }

    std::ifstream infile(filename);
    if (!infile.is_open()) {
      throw std::runtime_error(fmt::format("File Not Found : {}", filename.string()));
    }

    std::string line;
    std::regex regexInclude("#include +[\"<](.*)[\">]");
    std::smatch matchInclude;

    while (getline(infile, line)) {

      if (std::regex_match(line, matchInclude, regexInclude)) {
        fmt::print("{}\n", line);
        dependencyGraph.insert({filename.stem().string(), matchInclude[1].str()});
        uniqueHeader.insert(filename.stem().string());
        uniqueHeader.insert(matchInclude[1].str());
      }
    }

  }

  // =================================================================================================
  // Rendering
  const int canvasSize = 2000;
  const float radius = canvasSize / 4.f;
  const Point center = canvasSize / 2.f * Point(1, 1);

  int nbPoint = uniqueHeader.size();

  std::unordered_map<std::string, Point> classesPoints;
  int index = 0;
  for (auto& elem : uniqueHeader) {
    const float phi = 2.f * index * pi / nbPoint;
    classesPoints[elem] = radius * Point(cos(phi), sin(phi)) + center;
    index++;
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, 99);
  std::vector<Bezier> chords;
  for (auto [k, v] : dependencyGraph) {
    fmt::print("key={} value={}\n", k, v);
    chords.emplace_back(classesPoints[k], center, center, classesPoints[v]);
  }

  if (!svg::saveTiling(filename, chords, canvasSize)) {
    spdlog::error("Failed to save in file");
    return EXIT_FAILURE;
  }

  std::chrono::duration<double, std::milli> elapsed_temp = std::chrono::high_resolution_clock::now() - start_temp;
  fmt::print("Execution time: {:.2f} ms \n", elapsed_temp.count());

  return EXIT_SUCCESS;

} catch (const cxxopts::OptionException &e) {
  spdlog::error("Parsing options : {}", e.what());
  return EXIT_FAILURE;

} catch (const std::exception &e) {
  spdlog::error("{}", e.what());
  return EXIT_FAILURE;
}
