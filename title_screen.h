#pragma once

#include "screen.h"
#include "text.h"

class TitleScreen : public Screen {
  public:

    TitleScreen();

    bool update(const Input&, Audio&, unsigned int) override;
    void draw(Graphics&) const override;

    Screen* next_screen() const override;
    std::string get_music_track() const override { return "title.ogg"; }

  private:

    Text text_;

};
