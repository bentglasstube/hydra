#include "game_screen.h"

#include "util.h"

#include "components.h"
#include "config.h"

namespace {
  const polygon make_ship_shape(float size) {
    return {
      pos::polar(size, 0.0f),
      pos::polar(size / 3.0f, M_PI / 2),
      pos::polar(size / 3.0f, -M_PI / 2),
    };
  }
}

GameScreen::GameScreen() : rng_(Util::random_seed()), text_("text.png"), state_(state::playing), score_(0) {
  const auto player = reg_.create();
  reg_.emplace<Color>(player, 0xd8ff00ff);
  reg_.emplace<Polygon>(player, make_ship_shape(20.0f));
  reg_.emplace<Position>(player, pos{ kConfig.graphics.width / 2.0f, kConfig.graphics.height / 2.0f });

  reg_.emplace<PlayerControl>(player);
  reg_.emplace<ScreenWrap>(player);

  reg_.emplace<Acceleration>(player);
  reg_.emplace<Velocity>(player, 0.0f);
  reg_.emplace<Angle>(player, 0.0f);
  reg_.emplace<Rotation>(player);

  reg_.emplace<Health>(player, 100);

  spawn_drones(3, 5000.0f);
  spawn_asteroid(200.0f);
  spawn_asteroid(200.0f);
  spawn_asteroid(200.0f);
}

bool GameScreen::update(const Input& input, Audio& audio, unsigned int elapsed) {
  const float t = elapsed / 1000.0f;
  expiring(t);

  if (state_ == state::paused) {
    if (input.key_pressed(Input::Button::Start)) {
      state_ = state::playing;
    }
  } else {
    if (state_ == state::playing) {
      if (input.key_pressed(Input::Button::Start)) {
        state_ = state::paused;
        return true;
      }

      user_input(input);
      firing(audio, t);

      if (reg_.view<PlayerControl>().size() == 0) {
        // player must be dead
        state_ = state::lost;
        audio.play_sample("dead.wav");
      }
    }

    // movement systems
    acceleration(t);
    rotation(t);
    spin(t);
    steering(t);
    flocking();
    stay_in_bounds();
    max_velocity();
    movement(t);

    collision(audio);

    // cleanup
    kill_dead(audio);
    kill_oob();

  }

  return true;
}

namespace {
  constexpr bool oob(pos p) {
    if (p.x < 0 || p.x > kConfig.graphics.width) return true;
    if (p.y < 0 || p.y > kConfig.graphics.height) return true;
    return false;
  }
}

void GameScreen::kill_dead(Audio& audio) {
  auto view = reg_.view<const Health, const Position, const Color>();
  for (const auto e : view) {
    if (view.get<const Health>(e).health <= 0) {
      const pos p = view.get<const Position>(e).p;
      if (reg_.all_of<Crumble>(e)) {
        const float s = reg_.get<const Crumble>(e).size;
        spawn_asteroid_at(p, s);
        spawn_asteroid_at(p, s);
        spawn_asteroid_at(p, s);
      } else {
        spawn_drones(2, 5000.0f);
      }

      explosion(p, view.get<const Color>(e).color);
      audio.play_random_sample("boom.wav", 5);
      if (reg_.all_of<Killed>(e)) ++score_;
      reg_.destroy(e);
    }
  }
}

void GameScreen::kill_oob() {
  auto view = reg_.view<const Position, const KillOffScreen>();
  for (const auto e : view) {
    if (oob(view.get<const Position>(e).p)) reg_.destroy(e);
  }
}

namespace {
  uint32_t color_opacity(uint32_t color, float opacity) {
    const uint32_t lsb = (uint32_t)((color & 0xff) * std::clamp(opacity, 0.0f, 1.0f));
    return (color & 0xffffff00) | lsb;
  }

  void text_box(Graphics& graphics, const Text& text, const std::string& msg) {
    static const int width = 50;
    static const int height = 20;

    const Graphics::Point p1 { graphics.width() / 2 - width, graphics.height() / 2 - height };
    const Graphics::Point p2 { graphics.width() / 2 + width, graphics.height() / 2 + height };

    graphics.draw_rect(p1, p2, 0x000000ff, true);
    graphics.draw_rect(p1, p2, 0xffffffff, false);
    text.draw(graphics, msg, graphics.width() / 2, graphics.height() / 2 - 8, Text::Alignment::Center);
  }

  void health_box(Graphics& graphics, const Graphics::Point p1, const Graphics::Point p2, uint32_t color, float fullness) {
    graphics.draw_rect(p1, p2, 0x000000ff, true);
    graphics.draw_rect(p1, { p1.x + (int)((p2.x - p1.x) * fullness), p2.y }, color, true);
    graphics.draw_rect(p1, p2, color, false);
  }

  void draw_poly(Graphics& graphics, const polygon& poly, uint32_t color) {
    for (size_t i = 1; i < poly.points.size(); ++i) {
      const Graphics::Point p1 = { (int)poly.points[i - 1].x, (int)poly.points[i - 1].y };
      const Graphics::Point p2 = { (int)poly.points[i].x, (int)poly.points[i].y };
      graphics.draw_line(p1, p2, color);
    }
  }
}

void GameScreen::draw(Graphics& graphics) const {
  draw_flash(graphics);
  draw_polys(graphics);
  draw_bullets(graphics);
  draw_particles(graphics);
  draw_overlay(graphics);
}

void GameScreen::draw_flash(Graphics& graphics) const {
  const auto flashes = reg_.view<const Flash, const Timer, const Color>();
  for (const auto f : flashes) {
    const uint32_t c = color_opacity(flashes.get<const Color>(f).color, 1 - (flashes.get<const Timer>(f).ratio()));
    graphics.draw_rect({0, 0}, {graphics.width(), graphics.height()}, c, true);
  }
}

void GameScreen::draw_polys(Graphics& graphics) const {
  const auto polys = reg_.view<const Position, const Angle, const Polygon, const Color>();
  for (const auto p : polys) {
    const pos t = polys.get<const Position>(p).p;
    float a = polys.get<const Angle>(p).angle;
    if (reg_.all_of<Spin>(p)) a += reg_.get<const Spin>(p).dir;
    const polygon s = polys.get<const Polygon>(p).poly.translate(t, a);
    draw_poly(graphics, s, polys.get<const Color>(p).color);
  }
}

void GameScreen::draw_bullets(Graphics& graphics) const {
  const auto bullets = reg_.view<const Position, const Bullet>();
  for (const auto b : bullets) {
    const pos p = bullets.get<const Position>(b).p;
    graphics.draw_circle({ (int)p.x, (int)p.y}, 2, 0xffffffff, true);
  }
}

void GameScreen::draw_particles(Graphics& graphics) const {
  const auto particles = reg_.view<const Particle, const Timer, const Position, const Color>();
  for (const auto pt : particles) {
    const pos p = particles.get<const Position>(pt).p;
    graphics.draw_pixel({ (int)p.x, (int)p.y }, color_opacity(particles.get<const Color>(pt).color, 1 - particles.get<const Timer>(pt).ratio()));
  }
}

void GameScreen::draw_overlay(Graphics& graphics) const {
  const auto fade = reg_.view<const FadeOut, const Timer, const Color>();
  for (const auto f : fade) {
    const uint32_t c = color_opacity(fade.get<const Color>(f).color, fade.get<const Timer>(f).ratio());
    graphics.draw_rect({0, 0}, {graphics.width(), graphics.height()}, c, true);
  }

  if (state_ == state::paused) {
    graphics.draw_rect({0, 0}, {graphics.width(), graphics.height()}, 0x00000099, true);
    text_box(graphics, text_, "Paused");
  } else if (state_ == state::lost) {
    text_box(graphics, text_, "Game Over");
  }

  text_.draw(graphics, std::to_string(score_), graphics.width(), 0, Text::Alignment::Right);

  const auto players = reg_.view<const PlayerControl, const Color, const Health>();
  for (const auto p : players) {
    const Graphics::Point start {0, graphics.height() - 16};
    const Graphics::Point end {graphics.width(), graphics.height()};
    health_box(graphics, start, end, players.get<const Color>(p).color, players.get<const Health>(p).health / 100.0f);
  }
}

void GameScreen::user_input(const Input& input) {
  auto players = reg_.view<const PlayerControl, Acceleration, Rotation>();
  for (auto p : players) {
    float& accel = players.get<Acceleration>(p).accel;
    float& rot = players.get<Rotation>(p).rot;

    accel = 0.0f;
    if (input.key_held(Input::Button::Up)) accel += 1000.0f;
    if (input.key_held(Input::Button::Down)) accel -= 200.0f;

    rot = 0.0f;
    if (input.key_held(Input::Button::Left)) rot -= 2.0f;
    if (input.key_held(Input::Button::Right)) rot += 2.0f;

    if (input.key_held(Input::Button::A)) {
      static_cast<void>(reg_.get_or_emplace<Firing>(p));
    } else {
      reg_.remove<Firing>(p);
    }
  }
}

#define get_shape(v, e) v.get<const Polygon>(e).poly.translate(v.get<const Position>(e).p, v.get<const Angle>(e).angle)

void GameScreen::collision(Audio& audio) {
  auto objects = reg_.view<const PlayerControl, const Position, const Angle, const Polygon, Health>();
  for (auto o : objects) {
    const auto os = get_shape(objects, o);
    auto targets = reg_.view<const Collision, const Position, const Angle, const Polygon, Health>();
    for (auto t : targets) {
      if (o == t) continue;
      const auto ts = get_shape(targets, t);

      if (ts.intersect(os)) {
        objects.get<Health>(o).health--;
        targets.get<Health>(t).health--;

        // knockback
        const pos op = objects.get<const Position>(o).p;
        const pos tp = targets.get<const Position>(t).p;
        reg_.emplace_or_replace<Bump>(o, (op - tp).angle());
        reg_.emplace_or_replace<Bump>(t, (tp - op).angle());

        const auto flash = reg_.create();
        reg_.emplace<Flash>(flash);
        reg_.emplace<Timer>(flash, 0.2f);
        reg_.emplace<Color>(flash, (uint32_t)0x77000033);

        audio.play_random_sample("hurt.wav", 4);
      }
    }
  }

  auto bullets = reg_.view<const Bullet, const Position>();
  for (auto b : bullets) {
    const pos p = bullets.get<const Position>(b).p;
    auto targets = reg_.view<const Collision, const Polygon, const Position, const Angle, Health>();
    for (auto t : targets) {
      if (t == bullets.get<const Bullet>(b).source) continue;
      const auto ts = get_shape(targets, t);
      if (ts.contains(p)) {
        int& health = targets.get<Health>(t).health;
        if (--health == 0) reg_.emplace_or_replace<Killed>(t);
        audio.play_random_sample("hit.wav", 5);
        reg_.destroy(b);
        break;
      }
    }
  }
}

void GameScreen::acceleration(float t) {
  auto view = reg_.view<Velocity, const Acceleration>();
  for (const auto e : view) {
    float& vel = view.get<Velocity>(e).vel;
    const float friction = 0.01 * vel * vel * (vel < 0 ? -1 : 1);
    vel += (view.get<const Acceleration>(e).accel - friction) * t;
  }
}

void GameScreen::rotation(float t) {
  auto view = reg_.view<Angle, const Rotation>();
  for (const auto e : view) {
    float &angle = view.get<Angle>(e).angle;
    angle += view.get<const Rotation>(e).rot * t;
  }
}

void GameScreen::spin(float t) {
  auto view = reg_.view<Spin>();
  for (const auto e : view) {
    auto& s = view.get<Spin>(e);
    s.dir += s.spin * t;
  }
}

void GameScreen::steering(float t) {
  auto view = reg_.view<Angle, const TargetDir>();
  for (const auto e : view) {
    float &a = view.get<Angle>(e).angle;
    a += std::clamp(view.get<const TargetDir>(e).target - a, -t, t);
  }
}

void GameScreen::flocking() {
  auto view = reg_.view<const Flocking, const Position, Velocity, const Angle>();
  for (const auto e : view) {
    const pos boid = view.get<const Position>(e).p;
    const float angle = view.get<const Angle>(e).angle;
    float& vel = view.get<Velocity>(e).vel;

    size_t count = 0;
    pos center, flock;

    auto nearby = reg_.view<const Flocking, const Position, const Velocity, const Angle>();
    for (const auto o : nearby) {
      if (o == e) continue;
      pos p = nearby.get<const Position>(o).p;
      const float d = p.dist2(boid);

      // close enough to see
      if (d < 75.0f * 75.0f) {
        ++count;
        center += p;
        flock += pos::polar(nearby.get<const Velocity>(o).vel, nearby.get<const Angle>(o).angle);
      }
    }

    // avoid anything too close
    pos avoid;
    auto obstacles = reg_.view<const Collision, const Position>();
    for (const auto o : obstacles) {
      if (o == e) continue;
      const pos p = obstacles.get<const Position>(o).p;
      if (p.dist2(boid) < 50.0f * 50.0f) avoid += boid - p;
    }

    if (count > 0) {
      center /= count;
      flock /= count;

      const pos delta = (center - boid) * 0.005f + avoid * 0.25f + flock * 0.05f;
      const pos v = pos::polar(vel, angle) + delta;

      // only set the target direction otherwise the ships will awkwardly speed up and slow down
      reg_.emplace_or_replace<TargetDir>(e, v.angle());
    } else {
      // try to find the player
      auto players = reg_.view<const PlayerControl, const Position>();
      for (const auto p : players) {
        const pos seek = players.get<const Position>(p).p - boid;
        reg_.emplace_or_replace<TargetDir>(e, seek.angle());
        break;
      }
    }
  }
}

void GameScreen::stay_in_bounds() {
  const float buffer = 25.0f;

  auto view = reg_.view<const StayInBounds, const Position, Velocity, Angle>();
  for (const auto e : view) {
    const pos p = view.get<const Position>(e).p;
    float& vel = view.get<Velocity>(e).vel;
    float& angle = view.get<Angle>(e).angle;

    pos v = pos::polar(vel, angle);

    if (p.x < buffer) v.x = std::abs(v.x);
    if (p.x > kConfig.graphics.width - buffer) v.x = -std::abs(v.x);
    if (p.y < buffer) v.y = std::abs(v.y);
    if (p.y > kConfig.graphics.height - buffer) v.y = -std::abs(v.y);

    vel = v.mag();
    angle = v.angle();
  }
}

void GameScreen::max_velocity() {
  auto view = reg_.view<Velocity, const MaxVelocity>();
  for (const auto e : view) {
    float& vel = view.get<Velocity>(e).vel;
    const float max = view.get<const MaxVelocity>(e).max;
    if (vel > max) vel = max;
  }
}

void GameScreen::movement(float t) {
  auto view = reg_.view<Position, const Velocity, const Angle>();
  for (const auto e : view) {
    pos& p = view.get<Position>(e).p;
    const float vel = view.get<const Velocity>(e).vel;
    const float angle = view.get<const Angle>(e).angle;
    p += pos::polar(vel, angle) * t;

    if (reg_.all_of<ScreenWrap>(e)) {
      while (p.x < 0) p.x += kConfig.graphics.width;
      while (p.x > kConfig.graphics.width) p.x -= kConfig.graphics.width;
      while (p.y < 0) p.y += kConfig.graphics.height;
      while (p.y > kConfig.graphics.height) p.y -= kConfig.graphics.height;
    }

    if (reg_.all_of<Bump>(e)) {
      auto& b = reg_.get<Bump>(e);
      p += pos::polar(b.vel, b.dir);
      b.vel -= 1.0f * t;
      if (b.vel <= 0) reg_.remove<Bump>(e);
    }
  }
}

void GameScreen::expiring(float t) {
  auto view = reg_.view<Timer>();
  for (const auto e : view) {
    Timer& tm = view.get<Timer>(e);
    tm.elapsed += t;
    if (tm.expire && tm.elapsed > tm.lifetime) reg_.destroy(e);
  }
}

void GameScreen::firing(Audio& audio, float t) {
  auto sources = reg_.view<Firing, const Position, const Angle, const Velocity>();
  for (const auto s : sources) {
    Firing& gun = sources.get<Firing>(s);

    gun.time += t;
    if (gun.time > gun.rate) {
      gun.time -= gun.rate;
      const pos p = sources.get<const Position>(s).p;
      const float a = sources.get<const Angle>(s).angle;

      const auto bullet = reg_.create();
      reg_.emplace<Bullet>(bullet, s);
      reg_.emplace<Collision>(bullet);
      reg_.emplace<Position>(bullet, p + pos::polar(5, a));
      reg_.emplace<Angle>(bullet, a);
      reg_.emplace<Velocity>(bullet, sources.get<const Velocity>(s).vel + 250.0f);
      reg_.emplace<MaxVelocity>(bullet);
      reg_.emplace<KillOffScreen>(bullet);

      audio.play_random_sample("shot.wav", 3);
    }
  }
}

void GameScreen::spawn_drones(size_t count, float distance) {
  std::uniform_real_distribution<float> angle(0, 2 * M_PI);
  std::uniform_real_distribution<float> hue(175, 325);

  const pos center = {kConfig.graphics.width / 2.0f, kConfig.graphics.height / 2.0f};
  const pos p = center + pos::polar(distance, angle(rng_));
  const uint32_t c = hsl{hue(rng_), 1.0f, 0.5f};

  for (size_t i = 0; i < count; ++i) {
    const auto drone = reg_.create();
    reg_.emplace<Health>(drone, 1);
    reg_.emplace<Color>(drone, c);
    reg_.emplace<Polygon>(drone, make_ship_shape(15.0f));
    reg_.emplace<Position>(drone, p);
    reg_.emplace<Collision>(drone);
    reg_.emplace<Velocity>(drone, 200.0f);
    reg_.emplace<Angle>(drone, angle(rng_));
    reg_.emplace<MaxVelocity>(drone, 500.0f);
    reg_.emplace<StayInBounds>(drone);
    reg_.emplace<Flocking>(drone);
  }
}

void GameScreen::spawn_asteroid(float distance) {
  std::uniform_real_distribution<float> angle(0, 2 * M_PI);
  std::uniform_real_distribution<float> px(0, kConfig.graphics.width);
  std::uniform_real_distribution<float> py(0, kConfig.graphics.height);

  const pos center = {kConfig.graphics.width / 2.0f, kConfig.graphics.height / 2.0f};
  const pos p = center + pos::polar(distance, angle(rng_));
  const pos t = {px(rng_), py(rng_)};

  const auto roid = spawn_asteroid_at(p, 80.0f);
  reg_.get<Angle>(roid).angle = (t - p).angle();
}

entt::entity GameScreen::spawn_asteroid_at(pos p, float size) {
  std::uniform_real_distribution<float> angle(0, 2 * M_PI);
  std::uniform_int_distribution<size_t> sides(5, 11);
  std::uniform_real_distribution<float> wiggle(-size / 4.0f, size / 4.0f);
  std::uniform_real_distribution<float> vel(800.0f, 4000.0f);
  std::uniform_real_distribution<float> sat(0.0f, 0.8f);
  std::uniform_real_distribution<float> spin(-0.75f, 0.75f);

  const size_t side_count = sides(rng_);
  polygon poly;
  for (size_t i = 0; i < side_count; ++i) {
    const pos w = { wiggle(rng_), wiggle(rng_) };
    poly.points.emplace_back(pos::polar(size, 2 * M_PI * (float)i / (float)side_count) + w);
  }
  poly.points.emplace_back(poly.points[0]);

  const pos offset = { wiggle(rng_) * 4.0f, wiggle(rng_) * 4.0f };

  const auto roid = reg_.create();
  reg_.emplace<Color>(roid, hsl{45, sat(rng_), 0.7f});
  reg_.emplace<Polygon>(roid, poly);
  reg_.emplace<Position>(roid, p + offset);
  reg_.emplace<ScreenWrap>(roid);
  reg_.emplace<Collision>(roid);
  reg_.emplace<Velocity>(roid, vel(rng_) / size);
  reg_.emplace<Angle>(roid, angle(rng_));
  reg_.emplace<Spin>(roid, spin(rng_));
  reg_.emplace<Health>(roid, (int)size / 10);
  if (size > 10.0f) reg_.emplace<Crumble>(roid, size / 2.0f);

  return roid;
}

void GameScreen::explosion(const pos& p, uint32_t color) {
  std::uniform_real_distribution<float> angle(0, 2 * M_PI);
  std::uniform_real_distribution<float> vel(100.0f, 500.0f);
  std::uniform_real_distribution<float> lifetime(1.5f, 4.5f);

  for (size_t i = 0; i < 500; ++i) {
    const auto pt = reg_.create();
    reg_.emplace<Particle>(pt);
    reg_.emplace<Timer>(pt, lifetime(rng_));
    reg_.emplace<Position>(pt, p);
    reg_.emplace<Color>(pt, color);
    reg_.emplace<Velocity>(pt, vel(rng_));
    reg_.emplace<Angle>(pt, angle(rng_));
    reg_.emplace<StayInBounds>(pt);
  }
}
