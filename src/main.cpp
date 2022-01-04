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
#include <fstream>
#include <vector>
#include <filesystem>

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

  const int canvasSize = 2000;
  const float radius = canvasSize / 4.f;
  const Point center = canvasSize / 2.f * Point(1, 1);

  constexpr int nbPoint = 100;

  std::vector<Point> classesPoints;
  for (int i = 0; i < nbPoint; ++i) {
    const float phi = 2.f * i * pi / nbPoint;
    classesPoints.emplace_back(radius * Point(cos(phi), sin(phi)) + center);
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> distrib(0, 99);
  std::vector<Bezier> chords;
  for (int i = 0; i < nbPoint; ++i) {
    for (int j = 0; j < distrib(gen); ++j) {
      chords.emplace_back(classesPoints[i], center, center, classesPoints[distrib(gen)]);
    }
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
