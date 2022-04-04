#pragma once

#include "geometry.h"

struct Health { int health = 20; };

struct Position { pos p; };
struct Velocity { float vel = 0; };
struct Angle { float angle = 0; };
struct Bump { float dir, vel = 2.0f; };

struct MaxVelocity { float max = 3000.0f; };

struct Acceleration { float accel = 0.0f; };
struct Rotation { float rot = 0.0f; };
struct Spin { float spin = 0.0f, dir = 0.0f; };
struct TargetDir { float target = 0.0f; };

struct Color { uint32_t color = 0x006496ff; };

struct PlayerControl {};
struct Collision {};
struct Crumble { float size; };

struct Bullet { entt::entity source; };
struct Bomb { bool active = true; float time = 5.0f; };
struct Blast { float rad = 0.1f, fade = 1.5f; };
struct Firing { float rate = 0.175f, spread = 0.0f, time = rate; };
struct ScreenWrap {};
struct Polygon { polygon poly; };

struct Timer {
  float lifetime = 1.0f;
  bool expire = true;
  float elapsed = 0.0f;
  constexpr float ratio() const { return elapsed / lifetime; };
};

struct FadeOut {};
struct Flash {};
struct Particle {};

struct HasDrop {};
struct KilledByPlayer {};
struct KillOffScreen {};
struct BounceWalls {};
struct ReturnToField {};
struct SeekPlayer { float range = 25.0f; };
struct Flocking {};
