#pragma once

#include "geometry.h"

struct Health { int health = 20; };

struct Position { pos p; };
struct Size { float size = 0; };
struct Velocity { float vel = 0; };
struct Angle { float angle = 0; };

struct MaxVelocity { float max = 3000.0f; };

struct Acceleration { float accel = 0.0f; };
struct Rotation { float rot = 0.0f; };

struct Color { uint32_t color = 0x006496ff; };

struct PlayerControl {};

struct Timer {
  float lifetime = 1.0f;
  bool expire = true;
  float elapsed = 0.0f;
  constexpr float ratio() const { return elapsed / lifetime; };
};

struct FadeOut {};
