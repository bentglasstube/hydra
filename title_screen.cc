#include "title_screen.h"

#include "util.h"

#include "game_screen.h"

TitleScreen::TitleScreen() : text_("text.png", 16), title_("hydra.png", 5, 200, 200), space_(Util::random_seed()), counter_(0) {}

bool TitleScreen::update(const Input& input, Audio&, unsigned int elapsed) {
  const float t = elapsed / 1000.0f;
  counter_ += t;
  space_.update(10 * t);

  if (counter_ > 24.0f) {
    if (!dialog_) load_story_text();
    dialog_.update(t);

    if (dialog_.done()) {
      story_timeout_ -= t;
      if (story_timeout_ < 0) {
        ++story_text_;
        dialog_.dismiss();
      }
    }
  }

  return !input.any_pressed();
}

void TitleScreen::draw(Graphics& graphics) const {
  space_.draw(graphics);

  for (size_t i = 0; i < 5; ++ i) {
    const int x = graphics.width() / 2 - 500 + 200 * i;
    int y = 50 + 25 * std::sin((counter_ + i * M_PI) * 4 * M_PI);

    if (counter_ < i + 2) {
      y += (counter_ - i - 2) * 2500;
    }

    title_.draw(graphics, i, x, y);
  }

  dialog_.draw(graphics);

  if (counter_ > 8 && (int)(counter_ * 2) % 2 == 1) {
    text_.draw(graphics, "Press any key", graphics.width() / 2, graphics.height() - 100, Text::Alignment::Center);
  }
}

Screen* TitleScreen::next_screen() const {
  return new GameScreen;
}

void TitleScreen::load_story_text() {
  switch (story_text_) {
    case 0:
      dialog_.set_message("You are the last human survivor of an alien invasion.  You\nhave fled Earth in your spaceship and are being pursued by\nthe swarming drones.  While navigating an asteroid field,\nyour ship was hit, destroying your main engine.  You have\nonly your thrusters left and can no longer run.");
      break;

    case 1:
      dialog_.set_message("Trapped in the asteroid field, you make your last stand.\nEach drone you destroy makes more appear like the heads of\nthe mythical Hydra.  You can't win, but you can do as much\ndamage as possible before you go.");
      break;

    default:
      story_text_ = 0;
  }

  story_timeout_ = 24.0f;
}
