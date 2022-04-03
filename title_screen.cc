#include "title_screen.h"

#include "util.h"

#include "game_screen.h"

TitleScreen::TitleScreen() : text_("text.png", 16), title_("hydra.png", 5, 200, 200), space_(Util::random_seed()), counter_(0) {}

bool TitleScreen::update(const Input& input, Audio&, unsigned int elapsed) {
  counter_ += elapsed / 1000.0f;
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

  if (counter_ > 8 && (int)(counter_ * 2) % 2 == 1) {
    text_.draw(graphics, "Press any key", graphics.width() / 2, graphics.height() - 100, Text::Alignment::Center);
  }
}

Screen* TitleScreen::next_screen() const {
  return new GameScreen;
}
