#include "space.h"

#include "config.h"

Space::Space(uint64_t seed) : rng_(seed) {
  std::uniform_real_distribution<float> px(0, (float)kConfig.graphics.width);
  std::uniform_real_distribution<float> py(0, (float)kConfig.graphics.height);
  std::uniform_real_distribution<float> hue(0, 360);
  std::uniform_int_distribution<int> layer(1, 6);

  for (size_t i = 0; i < 1500; ++i) {
    stars_.push_back({px(rng_), py(rng_), layer(rng_), hsl{hue(rng_), 1.0f, 0.85f}});
  }
}

void Space::update(float t) {
  offset_ += t;
}

void Space::draw(Graphics& graphics) const {
  for (const auto s : stars_) {
    const int px = (int)(s.x + offset_ * s.layer) % graphics.width();
    graphics.draw_pixel({px, (int)s.y}, s.color);
  }
}
