//
//  https://github.com/edmBernard/include-graph
//
//  Created by Erwan BERNARD on 11/09/2021.
//
//  Copyright (c) 2021 Erwan BERNARD. All rights reserved.
//  Distributed under the Apache License, Version 2.0. (See accompanying
//  file LICENSE or copy at http://www.apache.org/licenses/LICENSE-2.0)
//

#pragma once

#include <geometry.hpp>

#include <spdlog/spdlog.h>

#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace svg {
namespace details {

std::string to_path(const Triangle &tr) {
  return fmt::format("M {} {} L {} {} L {} {} Z", tr.vertices[2].x, tr.vertices[2].y, tr.vertices[0].x, tr.vertices[0].y, tr.vertices[1].x, tr.vertices[1].y);
}

std::string to_path(const Quadrilateral &tr) {
  return fmt::format("M {} {} L {} {} L {} {} L {} {} Z", tr.vertices[0].x, tr.vertices[0].y, tr.vertices[1].x, tr.vertices[1].y, tr.vertices[3].x, tr.vertices[3].y, tr.vertices[2].x, tr.vertices[2].y);
}

std::string to_path(const Bezier &bz) {
  return fmt::format("M {} {} C {} {}, {} {}, {} {}",
                     bz.points[0].x, bz.points[0].y,
                     bz.points[1].x, bz.points[1].y,
                     bz.points[2].x, bz.points[2].y,
                     bz.points[3].x, bz.points[3].y);
}

} // namespace details

struct RGB {
  int r;
  int g;
  int b;

  RGB(int r, int g, int b)
      : r(r), g(g), b(b) {
  }
  RGB(uint32_t hexColor)
      : r((hexColor & 0xFF0000) >> 16), g((hexColor & 0x00FF00) >> 8), b(hexColor & 0x0000FF) {
  }
};

RGB operator+(const RGB &c1, const RGB &c2) {
  return {c1.r + c2.r, c1.g + c2.g, c1.b + c2.b};
}
RGB operator-(const RGB &c1, const RGB &c2) {
  return {c1.r - c2.r, c1.g - c2.g, c1.b - c2.b};
}
RGB operator*(float value, const RGB &c) {
  return {int(value * c.r), int(value * c.g), int(value * c.b)};
}
float norm(const RGB &c1) {
  return c1.r * c1.r + c1.g * c1.g + c1.b * c1.b;
}

struct Fill {
  int r;
  int g;
  int b;

  Fill(int r, int g, int b)
      : r(r), g(g), b(b) {
  }
  Fill(RGB rgb)
      : r(rgb.r), g(rgb.g), b(rgb.b) {
  }
  Fill(uint32_t hexColor)
      : Fill(RGB(hexColor)) {
  }
};

struct Strokes {
  int r;
  int g;
  int b;
  float width;

  Strokes(int r, int g, int b, float width)
      : r(r), g(g), b(b), width(width) {
  }
  Strokes(RGB rgb, float width)
      : r(rgb.r), g(rgb.g), b(rgb.b), width(width) {
  }
  Strokes(uint32_t hexColor, float width)
      : Strokes(RGB(hexColor), width) {
  }
};

std::string to_path(
    const std::vector<Bezier> &lines,
    RGB firstColor, RGB lastColor, float width) {

  std::string output = fmt::format(
    "<defs>\n"
    "<linearGradient id='FirstGradient' >\n"
    "<stop offset='0%' style='stop-color:rgb({},{},{});'/>\n"
    "<stop offset='100%' style='stop-color:rgb({},{},{});'/>\n"
    "</linearGradient>\n"
    "</defs>\n", firstColor.r, firstColor.g, firstColor.b, lastColor.r, lastColor.g, lastColor.b);

  const std::string s_strockes = fmt::format("stroke:url(#FirstGradient);stroke-width:{};stroke-opacity:0.1;stroke-linecap:butt;stroke-linejoin:round", width);
  for (auto &bz : lines) {
    // gradient is fixed, to applied on line in function of the orientation,
    // we rotate the line horizontaly apply the gradient and rotate the line back to it's position
    const float cosPhi = scalar(Point(1, 0), (bz.points[0] - bz.points[3]) / norm(bz.points[0] - bz.points[3]));
    const float sinPhi = scalar(Point(0, 1), (bz.points[0] - bz.points[3]) / norm(bz.points[0] - bz.points[3]));
    const float angle = sinPhi > 0 ? std::acos(cosPhi) : 2 * pi - std::acos(cosPhi);
    output += fmt::format("<path style='{};fill:none' transform='rotate({})' d='{}'></path>\n ", s_strockes, angle * 180.f / pi, details::to_path(rotate(bz, - angle)));
  }
  return output;
}

template <typename Geometry, typename Point>
[[nodiscard]] bool saveTiling(const std::string &filename,
                              const std::vector<Geometry> &lines,
                              const std::unordered_map<std::string, Point> &labels,
                              int canvasSize) {

  std::ofstream out(filename);
  if (!out) {
    spdlog::error("Cannot open output file : {}.", filename);
    return false;
  }

  out << "<svg xmlns='http://www.w3.org/2000/svg' "
      << fmt::format("height='{size}' width='{size}' viewBox='0 0 {size} {size}'>\n", fmt::arg("size", canvasSize))
      << fmt::format("<rect height='100%' width='100%' fill='rgb({},{},{})'/>\n", 0, 0, 0)
      << "<g id='surface1'>\n"
      ;

  out << to_path(lines, 0x0044C2, 0xEB0041, 1);

  const RGB textColor(0xffffff);
  const std::string s_fill = fmt::format("fill:rgb({},{},{})", textColor.r, textColor.g, textColor.b);
  for (auto& [k, v] : labels) {
    out << fmt::format("<text style='{}' font-size='0.5em' dy='0.25em' transform='translate({},{}) rotate({})'>{}</text>\n", s_fill, v.x, v.y, v.angle * 180.f / pi, k);
  }
  out << "</g>\n</svg>\n";
  return true;
}

} // namespace svg
