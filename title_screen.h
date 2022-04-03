#pragma once

#include "screen.h"
#include "spritemap.h"
#include "text.h"

#include "game_screen.h"
#include "space.h"

class TitleScreen : public Screen {
  public:

    TitleScreen();

    bool update(const Input&, Audio&, unsigned int) override;
    void draw(Graphics&) const override;

    Screen* next_screen() const override;
    std::string get_music_track() const override { return "title.ogg"; }

  private:

    Text text_;
    SpriteMap title_;
    Space space_;

    float counter_;

};
