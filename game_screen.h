#pragma once

#include <random>

#include "entt/entity/registry.hpp"

#include "screen.h"
#include "text.h"

#include "geometry.h"

class GameScreen : public Screen {
  public:

    GameScreen();

    bool update(const Input& input, Audio& audio, unsigned int elasped) override;
    void draw(Graphics& graphics) const override;

    Screen* next_screen() const override;
    std::string get_music_track() const override { return "battle.ogg"; }

  private:

    enum class state { playing, paused, lost };

    entt::registry reg_;
    std::mt19937 rng_;
    Text text_;

    state state_;
    int score_, combo_, best_combo_;
    float spawns_, spawn_timer_;
    float roid_timer_;

    void user_input(const Input& input);

    void collision(Audio& audio);

    void acceleration(float t);
    void rotation(float t);
    void spin(float t);
    void steering(float t);
    void flocking();
    void stay_in_bounds();
    void max_velocity();
    void movement(float t);

    void expiring(float t);
    void firing(Audio& audio, float t);

    void kill_dead(Audio& audio);
    void kill_oob();

    void draw_flash(Graphics& graphics) const;
    void draw_polys(Graphics& graphics) const;
    void draw_bullets(Graphics& graphics) const;
    void draw_particles(Graphics& graphics) const;
    void draw_overlay(Graphics& graphics) const;

    void spawn_drones(size_t count, float distance);
    void spawn_asteroid(float distance);
    entt::entity spawn_asteroid_at(pos p, float size);
    void explosion(const pos& p, uint32_t color);
};
