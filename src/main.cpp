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
#include <regex>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fs = std::filesystem;

bool isValidExtension(const std::string& ext) {
  std::vector<std::string> validExtention = {".cpp", ".hpp", ".h"};
  for (auto v : validExtention) {
    if (v == ext) {
      return true;
    }
  }
  return false;
}

struct PointWithAngle : public Point {
  PointWithAngle() {}
  PointWithAngle(float x, float y, float angle)
      : Point(x, y), angle(angle) {}
  PointWithAngle(Point pt, float angle)
      : Point(pt), angle(angle) {}
  float angle = 0;
};

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
    ("exclude", "Exclude pattern", cxxopts::value<std::string>()->default_value(""))
    ("ignore-external", "Ignore include outside of the source folder", cxxopts::value<bool>())
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

  const std::string excludePattern = clo["exclude"].as<std::string>();
  const std::regex regexExclude(excludePattern);

  const bool ignoreExternal = clo.count("ignore-external");

  // =================================================================================================
  // Code
  auto start_temp = std::chrono::high_resolution_clock::now();

  // =================================================================================================
  // Parsing
  std::unordered_multimap<std::string, std::string> dependencyGraph;
  std::unordered_map<std::string, std::vector<std::string>> headerByFolder;
  std::set<std::string> uniqueHeader;

  std::unordered_set<std::string> allFilesAbsolute;
  std::unordered_set<std::string> allFilesStem;
  for (auto& p : fs::recursive_directory_iterator(inputFolder)) {

    const fs::path filename = p.path();
    if (!fs::is_regular_file(filename)) {
      continue;
    }
    if (!isValidExtension(filename.extension().string())) {
      continue;
    }
    std::string absolutePath = std::filesystem::absolute(filename).string();
    if (std::regex_match(absolutePath, regexExclude)) {
      continue;
    }
    allFilesAbsolute.insert(absolutePath);
    allFilesStem.insert(filename.stem().string());
    headerByFolder[filename.parent_path().string()].push_back(filename.stem().string());

  }

  for (auto& p : allFilesAbsolute) {
    const fs::path filename = fs::path(p);

    std::ifstream infile(filename);
    if (!infile.is_open()) {
      throw std::runtime_error(fmt::format("File Not Found : {}", filename.string()));
    }

    std::string line;
    std::regex regexInclude("#include +[\"<](.*)[\">]");
    std::smatch matchInclude;

    while (getline(infile, line)) {

      if (std::regex_match(line, matchInclude, regexInclude)) {
        const std::string includeFilename = fs::path(matchInclude[1].str()).stem().string();
        const std::string includeExtension = fs::path(matchInclude[1].str()).extension().string();

        if (ignoreExternal && !allFilesStem.count(includeFilename)) {
          continue;
        }
        if (std::regex_match(includeFilename, regexExclude)) {
          continue;
        }
        dependencyGraph.insert({filename.stem().string(), includeFilename});
        uniqueHeader.insert(filename.stem().string());
        uniqueHeader.insert(includeFilename);
      }
    }

  }

  // =================================================================================================
  // Rendering
  const int canvasSize = 2000;
  const float radius = canvasSize / 4.f;
  const Point center = canvasSize / 2.f * Point(1, 1);

  constexpr int spacing = 2;
  int nbPoint = uniqueHeader.size() + spacing * headerByFolder.size() + 1;

  std::unordered_map<std::string, PointWithAngle> classesPoints;
  int index = 0;
  auto addLabels = [&](const std::string& elem) {
    const float phi = 2.f * index * pi / nbPoint;
    classesPoints[elem] = PointWithAngle(radius * Point(cos(phi), sin(phi)) + center, phi);
    index++;
  };
  for (auto& [k, v] : headerByFolder) {
    for (auto& elem : v) {
      addLabels(elem);
    }
    index += spacing; // increment for spacing between folder
  }
  for (auto& elem : uniqueHeader) {
    if (!allFilesStem.count(elem)) {
      addLabels(elem);
    }
  }

  std::vector<Bezier> chords;
  for (auto [k, v] : dependencyGraph) {
    const float distance = norm(classesPoints[k] - classesPoints[v]);
    const Point tangentBegin = classesPoints[k] + (center - classesPoints[k]) * distance / (2 * radius);
    const Point tangentEnd = classesPoints[v] + (center - classesPoints[v]) * distance / (2 * radius);
    chords.emplace_back(classesPoints[k], tangentBegin, tangentEnd, classesPoints[v]);
  }

  if (!svg::saveTiling(filename, chords, classesPoints, canvasSize)) {
    spdlog::error("Failed to save in file");
    return EXIT_FAILURE;
  }

  std::chrono::duration<double, std::milli> elapsed_temp = std::chrono::high_resolution_clock::now() - start_temp;
  fmt::print("Number of curve: {} \n", uniqueHeader.size());
  fmt::print("Execution time: {:.2f} ms \n", elapsed_temp.count());

  return EXIT_SUCCESS;

} catch (const cxxopts::OptionException &e) {
  spdlog::error("Parsing options : {}", e.what());
  return EXIT_FAILURE;

} catch (const std::exception &e) {
  spdlog::error("{}", e.what());
  return EXIT_FAILURE;
}
