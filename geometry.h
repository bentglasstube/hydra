#pragma once

#include <vector>

struct pos {
  float x = 0, y = 0;

  constexpr pos operator+(const pos other) const { return { x + other.x, y + other.y}; }
  constexpr pos operator-(const pos other) const { return { x - other.x, y - other.y}; }
  constexpr pos operator*(const float n) const { return { x * n, y * n }; }
  constexpr pos operator/(const float n) const { return { x / n, y / n }; }

  constexpr pos& operator+=(const pos other) { x += other.x; y += other.y; return *this; }
  constexpr pos& operator-=(const pos other) { x -= other.x; y -= other.y; return *this; }
  constexpr pos& operator*=(const float n) { x *= n; y *= n; return *this; }
  constexpr pos& operator/=(const float n) { x /= n; y /= n; return *this; }

  constexpr bool operator==(const pos other) const { return x == other.x && y == other.y; }
  constexpr bool operator!=(const pos other) const { return x != other.x || y != other.y; }

  constexpr float dist2(const pos other) const {
    const float dx = x - other.x;
    const float dy = y - other.y;
    return dx * dx + dy * dy;
  }

  float angle() const { return std::atan2(y, x); }
  float mag() const { return std::sqrt(x * x + y * y); }

  static pos polar(float r, float theta) { return { r * std::cos(theta), r * std::sin(theta) }; }
};

struct rect {
  float left = 0, top = 0, right = 0, bottom = 0;

  constexpr float x() const { return left; }
  constexpr float y() const { return top; }
  constexpr float width() const { return right - left; }
  constexpr float height() const { return bottom - top; }
  constexpr float area() const { return width() * height(); }

  constexpr bool intersect(const rect other) const {
    return left < other.right && right > other.left && top < other.bottom && bottom > other.top;
  }

  constexpr bool contains(const pos p) const {
    return left < p.x && right > p.x && top < p.y && bottom > p.y;
  }

};

namespace {
  int orientation(pos p, pos q, pos r) {
    const float v = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (v == 0) return 0;
    return v < 0 ? -1 : 1;
  }

  bool lines_intersect(pos p, pos q, pos r, pos s) {
    int o1 = orientation(p, q, r);
    int o2 = orientation(p, q, s);
    int o3 = orientation(r, s, p);
    int o4 = orientation(r, s, q);
    return o1 != o2 && o3 != o4;
  }
}

struct polygon {
  std::vector<pos> points;

  polygon() : points() {}
  polygon(std::initializer_list<pos> p) : points(p) { points.emplace_back(points[0]); }

  bool intersect(const polygon& other) const {
    for (size_t i = 1; i < points.size(); ++i) {
      for (size_t j = 1; j < other.points.size(); ++j) {
        const pos p = points[i - 1];
        const pos q = points[i];
        const pos r = other.points[j - 1];
        const pos s = other.points[j];

        if (lines_intersect(p, q, r, s)) return true;
      }
    }
    return false;
  }

  bool contains(const pos& p) const {
    const pos q = { 1000000.0f, p.y };
    size_t crossings = 0;
    for (size_t i = 1; i < points.size(); ++i) {
      const pos r = points[i - 1];
      const pos s = points[i];
      if (lines_intersect(p, q, r, s)) ++crossings;
    }
    return crossings % 2 == 1;
  }

  polygon translate(const pos& translate, float rotate) const {
    polygon other = {};
    for (const auto& p : points) {
      other.points.emplace_back(translate + pos::polar(p.mag(), p.angle() + rotate));
    }
    return other;
  }

};

namespace {
  uint32_t make_color(float r, float g, float b) {
    return (uint32_t)(std::round(255 * r)) << 24 |
           (uint32_t)(std::round(255 * g)) << 16 |
           (uint32_t)(std::round(255 * b)) << 8 |
           0xff;
  }
}

struct hsl {
  float hue = 0, sat = 0, light = 0;

  operator uint32_t() const {
    const float c = (1 - std::abs(2 * light - 1)) * sat;
    const float x = c * (1 - std::abs(std::fmod(hue / 60, 2) - 1));
    const float m = light - c / 2.0f;

    if (hue < 60)  return make_color(c + m, x + m, m);
    if (hue < 120) return make_color(x + m, c + m, m);
    if (hue < 180) return make_color(m, c + m, x + m);
    if (hue < 240) return make_color(m, x + m, c + m);
    if (hue < 300) return make_color(x + m, m, c + m);
    else           return make_color(x + m, m, x + m);
  };
};
