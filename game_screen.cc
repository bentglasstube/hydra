#include "game_screen.h"

#include "util.h"

#include "components.h"
#include "config.h"
#include "geometry.h"

GameScreen::GameScreen() : rng_(Util::random_seed()), text_("text.png"), state_(state::playing), score_(0) {
  const auto player = reg_.create();
  reg_.emplace<Color>(player, 0xd8ff00ff);
  reg_.emplace<Position>(player, pos{ kConfig.graphics.width / 2.0f, kConfig.graphics.height / 2.0f });
  reg_.emplace<PlayerControl>(player);
  reg_.emplace<Accelleration>(player);
  reg_.emplace<Velocity>(player, 0.0f);
  reg_.emplace<Angle>(player, 0.0f);
  reg_.emplace<Rotation>(player);
  reg_.emplace<Size>(player, 20.0f);
  reg_.emplace<Health>(player, 100);
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

      // movement systems
      accelleration(t);
      rotation(t);
      max_velocity();
      movement(t);

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
  rect get_rect(const pos& p, float size) {
    return { p.x - size / 2, p.y - size / 2, p.x + size / 2, p.y + size / 2 };
  }

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
}

void GameScreen::draw(Graphics& graphics) const {
  draw_ships(graphics);
  draw_overlay(graphics);
}

void GameScreen::draw_ships(Graphics& graphics) const {
  const auto ships = reg_.view<const Position, const Size, const Color, const Angle>();
  for (const auto s : ships) {
    const pos p = ships.get<const Position>(s).p;
    const float size = ships.get<const Size>(s).size;
    const rect r = get_rect(p, size);
    const bool filled = reg_.all_of<PlayerControl>(s);

    graphics.draw_rect({ (int)r.left, (int)r.top }, { (int)r.right, (int)r.bottom }, ships.get<const Color>(s).color, filled);
    const float angle = reg_.get<const Angle>(s).angle;
    graphics.draw_line({ (int)p.x, (int)p.y }, { (int)(p.x + size * std::cos(angle)), (int)(p.y + size * std::sin(angle)) }, 0xffffffff);
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
  auto players = reg_.view<const PlayerControl, Accelleration, Rotation>();
  for (auto p : players) {
    float& accel = players.get<Accelleration>(p).accel;
    float& rot = players.get<Rotation>(p).rot;

    accel = 0.0f;
    if (input.key_held(Input::Button::Up)) accel += 10.0f;
    if (input.key_held(Input::Button::Down)) accel -= 2.0f;

    rot = 0.0f;
    if (input.key_held(Input::Button::Left)) rot -= 1.0f;
    if (input.key_held(Input::Button::Right)) rot += 1.0f;
  }
}

void GameScreen::accelleration(float t) {
  auto view = reg_.view<Velocity, const Accelleration>();
  for (const auto e : view) {
    float& vel = view.get<Velocity>(e).vel;
    vel = (vel + view.get<const Accelleration>(e).accel * t) * 0.99;
  }
}

void GameScreen::rotation(float t) {
  auto view = reg_.view<Angle, const Rotation>();
  for (const auto e : view) {
    float &angle = view.get<Angle>(e).angle;
    angle += view.get<const Rotation>(e).rot * t;
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
