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

#include <fmt/format.h>

#include <array>
#include <optional>
#include <string>
#include <vector>

constexpr float pi = 3.14159265358979f;
constexpr float epsilon = 0.1f; // use for the comparison between Point

//------------------------------------------------------------------------------
// Point

struct Point {
  float x = 0;
  float y = 0;

  Point() {}
  Point(float x, float y)
      : x(x), y(y) {
  }
};

Point operator+(const Point &pt1, const Point &pt2) {
  return {pt1.x + pt2.x, pt1.y + pt2.y};
}
Point operator-(const Point &pt1, const Point &pt2) {
  return {pt1.x - pt2.x, pt1.y - pt2.y};
}
Point operator*(float value, const Point &pt) {
  return {value * pt.x, value * pt.y};
}
Point operator*(const Point &pt, float value) {
  return {value * pt.x, value * pt.y};
}
Point operator/(const Point &pt, float value) {
  return {pt.x / value, pt.y / value};
}
float scalar(const Point &pt1, const Point &pt2) {
  return pt1.x * pt2.x + pt1.y * pt2.y;
}
float norm(const Point &pt1) {
  return std::sqrt(pt1.x * pt1.x + pt1.y * pt1.y);
}
bool operator==(const Point &lhs, const Point &rhs) {
  return norm(lhs - rhs) < epsilon;
}
bool operator<(const Point &lhs, const Point &rhs) {
  if (abs(lhs.x - rhs.x) < epsilon)
    return lhs.y < rhs.y;
  return lhs.x < rhs.x;
}
Point rotate(const Point &lhs, float angle) {
  return { lhs.x * std::cos(angle) - lhs.y * std::sin(angle), lhs.x * std::sin(angle) + lhs.y * std::cos(angle)};
}

std::string to_string(const Point &pt) {
  return fmt::format("({}, {})", pt.x, pt.y);
}

//------------------------------------------------------------------------------
// Triangle
struct Triangle {
  std::array<Point, 3> vertices;

  Triangle(Point A, Point B, Point C)
      : vertices{A, B, C} {
  }

  Point center() {
    return (this->vertices[0] + this->vertices[1] + this->vertices[2]) / 3.;
  }
};

bool operator==(const Triangle &lhs, const Triangle &rhs) {
  return rhs.vertices[0] == lhs.vertices[0] &&
         rhs.vertices[1] == lhs.vertices[1] &&
         rhs.vertices[2] == lhs.vertices[2];
}

std::string to_string(const Triangle &triangle) {
  return fmt::format("{}, {}, {}", to_string(triangle.vertices[0]), to_string(triangle.vertices[1]), to_string(triangle.vertices[2]));
}

//------------------------------------------------------------------------------
// Quadrilateral
struct Quadrilateral {
  std::array<Point, 4> vertices;

  Quadrilateral(Point A, Point B, Point C, Point D)
      : vertices{A, B, C, D} {
  }

  Point center() const {
    return (this->vertices[0] + this->vertices[1] + this->vertices[2] + this->vertices[3]) / 4.;
  }
};

bool operator==(const Quadrilateral &lhs, const Quadrilateral &rhs) {
  // we compare gravity center approximative be enough
  return lhs.center() == rhs.center();
}

bool operator<(const Quadrilateral &lhs, const Quadrilateral &rhs) {
  // we compare gravity center approximative be enough
  return lhs.center() < rhs.center();
}

std::string to_string(const Quadrilateral &quad) {
  return fmt::format("{}, {}, {}, {}", to_string(quad.vertices[0]), to_string(quad.vertices[1]), to_string(quad.vertices[2]), to_string(quad.vertices[3]));
}

//------------------------------------------------------------------------------
// Bezier
struct Bezier {
  std::array<Point, 4> points;

  Bezier(Point begin, Point beginTangent, Point endTangent, Point end)
      : points{begin, beginTangent, endTangent, end} {
  }
};

Bezier rotate(const Bezier &lhs, float angle) {
  return {
    rotate(lhs.points[0], angle),
    rotate(lhs.points[1], angle),
    rotate(lhs.points[2], angle),
    rotate(lhs.points[3], angle)
    };
}

std::string to_string(const Bezier &bz) {
  return fmt::format("{}, {}, {}, {}",
    to_string(bz.points[0]), to_string(bz.points[1]), to_string(bz.points[2]), to_string(bz.points[3]));
}
