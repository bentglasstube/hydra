#include "game_screen.h"

#include "util.h"

#include "components.h"
#include "config.h"
#include "geometry.h"

GameScreen::GameScreen() : rng_(Util::random_seed()), text_("text.png"), state_(state::playing), score_(0) {
  const auto player = reg_.create();
  reg_.emplace<Color>(player, 0xd8ff00ff);
  reg_.emplace<Triangle>(player);
  reg_.emplace<Size>(player, 20.0f);

  reg_.emplace<Position>(player, pos{ kConfig.graphics.width / 2.0f, kConfig.graphics.height / 2.0f });

  reg_.emplace<PlayerControl>(player);
  reg_.emplace<ScreenWrap>(player);

  reg_.emplace<Acceleration>(player);
  reg_.emplace<Velocity>(player, 0.0f);
  reg_.emplace<Angle>(player, 0.0f);
  reg_.emplace<Rotation>(player);

  reg_.emplace<Health>(player, 100);

  spawn_drones(5);
}

bool GameScreen::update(const Input& input, Audio&, unsigned int elapsed) {
  const float t = elapsed / 1000.0f;
  expiring(t);

  switch (state_) {
    case state::playing:

      if (input.key_pressed(Input::Button::Start)) {
        state_ = state::paused;
        return true;
      }

      user_input(input);
      firing(t);

      // movement systems
      acceleration(t);
      rotation(t);
      stay_in_bounds();
      max_velocity();
      movement(t);

      collision();

      // cleanup
      kill_dead();
      kill_oob();

      if (reg_.view<PlayerControl>().size() == 0) {
        // player must be dead
        state_ = state::lost;

        const auto fade = reg_.create();
        reg_.emplace<FadeOut>(fade);
        reg_.emplace<Timer>(fade, 2.5f, false);
        reg_.emplace<Color>(fade, (uint32_t)0x000000ff);
      }

      break;

    case state::paused:

      if (input.key_pressed(Input::Button::Start)) {
        state_ = state::playing;
      }
      break;

    default:
      // nothing to do
      break;

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

void GameScreen::kill_dead() {
  auto view = reg_.view<const Health>();
  for (const auto e : view) {
    if (view.get<const Health>(e).health <= 0) {
      ++score_;
      reg_.destroy(e);
      spawn_drones(2);
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
  const polygon get_ship_shape(const pos& p, float angle, float size) {
    return {
      p + pos::polar(size, angle),
      p + pos::polar(size / 3.0f, angle + M_PI / 2),
      p + pos::polar(size / 3.0f, angle - M_PI / 2),
    };
  }

#define ship_shape(v, e) get_ship_shape(v.get<const Position>(e).p, v.get<const Angle>(e).angle, v.get<const Size>(e).size)

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
  draw_ships(graphics);
  draw_bullets(graphics);
  draw_overlay(graphics);
}

void GameScreen::draw_ships(Graphics& graphics) const {
  const auto ships = reg_.view<const Position, const Size, const Color, const Angle, const Triangle>();
  for (const auto s : ships) {
    draw_poly(graphics, ship_shape(ships, s), ships.get<const Color>(s).color);
  }
}

void GameScreen::draw_bullets(Graphics& graphics) const {
  const auto bullets = reg_.view<const Position, const Bullet>();
  for (const auto b : bullets) {
    const pos p = bullets.get<const Position>(b).p;
    graphics.draw_circle({ (int)p.x, (int)p.y}, 2, 0xffffffff, true);
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

void GameScreen::collision() {
  auto players = reg_.view<const PlayerControl, const Position, const Angle, const Size, const Triangle, Health>();
  for (auto p : players) {
    const auto ps = ship_shape(players, p);
    auto targets = reg_.view<const Collision, const Position, const Angle, const Size, const Triangle>();
    for (auto t : targets) {
      if (p == t) continue;
      const auto ts = ship_shape(targets, t);

      if (ts.intersect(ps)) {
        players.get<Health>(p).health--;
        reg_.destroy(t);
        spawn_drones(1);
      }
    }
  }

  auto bullets = reg_.view<const Bullet, const Position>();
  for (auto b : bullets) {
    const pos p = bullets.get<const Position>(b).p;
    auto targets = reg_.view<const Collision, const Position, const Angle, const Size, Health>();
    for (auto t : targets) {
      if (t == bullets.get<const Bullet>(b).source) continue;
      const auto ts = ship_shape(targets, t);
      if (ts.contains(p)) {
        targets.get<Health>(t).health--;
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
    vel = (vel + view.get<const Acceleration>(e).accel * t) * 0.99;
  }
}

void GameScreen::rotation(float t) {
  auto view = reg_.view<Angle, const Rotation>();
  for (const auto e : view) {
    float &angle = view.get<Angle>(e).angle;
    angle += view.get<const Rotation>(e).rot * t;
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

    if (p.x < buffer) v.x += 50.0f;
    if (p.x > kConfig.graphics.width - buffer) v.x -= 50.0f;
    if (p.y < buffer) v.y += 50.0f;
    if (p.y > kConfig.graphics.height - buffer) v.y -= 50.0f;

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

void GameScreen::firing(float t) {
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
      reg_.emplace<Position>(bullet, p + pos::polar(5, a));
      reg_.emplace<Angle>(bullet, a);
      reg_.emplace<Velocity>(bullet, sources.get<const Velocity>(s).vel + 250.0f);
      reg_.emplace<MaxVelocity>(bullet);
      reg_.emplace<KillOffScreen>(bullet);
    }
  }
}

void GameScreen::spawn_drones(size_t count) {
  std::uniform_int_distribution<int> px(0, kConfig.graphics.width);
  std::uniform_int_distribution<int> py(0, kConfig.graphics.height);
  std::uniform_real_distribution<float> angle(0, 2 * M_PI);

  for (size_t i = 0; i < count; ++i) {
    const auto drone = reg_.create();
    const pos p = { (float)px(rng_), (float)py(rng_) };

    reg_.emplace<Health>(drone, 1);
    reg_.emplace<Color>(drone, (uint32_t)0x00ffffff);
    reg_.emplace<Triangle>(drone);
    reg_.emplace<Position>(drone, p);
    reg_.emplace<Size>(drone, 15.0f);
    reg_.emplace<Collision>(drone);
    reg_.emplace<Velocity>(drone, 200.0f);
    reg_.emplace<Angle>(drone, angle(rng_));
    reg_.emplace<MaxVelocity>(drone, 500.0f);
    reg_.emplace<StayInBounds>(drone);
  }
}
