#include "space.h"

#include "config.h"

Space::Space(uint64_t seed) {
  std::uniform_int_distribution<int> px(0, kConfig.graphics.width);
  std::uniform_int_distribution<int> py(0, kConfig.graphics.height);
  std::uniform_real_distribution<float> hue(0, 360);

  for (size_t i = 0; i < 1024; ++i) {
    stars_.push_back({px(rng_), py(rng_), hsl{hue(rng_), 1.0f, 0.85f}});
  }
}

void Space::draw(Graphics& graphics) const {
  std::uniform_int_distribution<int> twinkle(0, 64);
  for (const auto s : stars_) {
    /* if (twinkle(rng_) != 0) */ graphics.draw_pixel({s.x, s.y}, s.color);
  }
}
