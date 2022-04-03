#pragma once

#include <random>
#include <vector>

#include "graphics.h"

#include "geometry.h"

class Space {
  public:

    Space(uint64_t seed);
    void update(float t);
    void draw(Graphics& graphics) const;

  private:

    struct Star {
      float x, y;
      int layer;
      uint32_t color;
    };

    mutable std::mt19937 rng_;
    std::vector<Star> stars_;
    float offset_;
};
