#pragma once

#include <random>
#include <vector>

#include "graphics.h"

#include "geometry.h"

class Space {
  public:

    Space(uint64_t seed);
    void draw(Graphics& graphics) const;

  private:

    struct Star {
      int x, y;
      uint32_t color;
    };

    std::mt19937 rng_;
    std::vector<Star> stars_;
};
