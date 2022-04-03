#pragma once

#include <random>

#include "entt/entity/registry.hpp"

#include "screen.h"
#include "text.h"

class GameScreen : public Screen {
  public:

    GameScreen();

    bool update(const Input& input, Audio& audio, unsigned int elasped) override;
    void draw(Graphics& graphics) const override;

  private:

    enum class state { playing, paused, lost };

    entt::registry reg_;
    std::mt19937 rng_;
    Text text_;

    state state_;
    int score_;

    void user_input(const Input& input);

    void collision();

    void acceleration(float t);
    void rotation(float t);
    void stay_in_bounds();
    void max_velocity();
    void movement(float t);

    void expiring(float t);
    void firing(float t);

    void kill_oob();

    void draw_ships(Graphics& graphics) const;
    void draw_bullets(Graphics& graphics) const;
    void draw_overlay(Graphics& graphics) const;

    void spawn_drones(size_t count = 1);
};
