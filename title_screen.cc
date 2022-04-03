#include "title_screen.h"

#include "game_screen.h"

TitleScreen::TitleScreen() : text_("text.png", 16), title_("hydra.png", 5, 200, 200), space_(8675309), counter_(0) {}

bool TitleScreen::update(const Input& input, Audio&, unsigned int elapsed) {
  counter_ += elapsed / 1000.0f;
  return !input.any_pressed();
}

void TitleScreen::draw(Graphics& graphics) const {
  space_.draw(graphics);

  for (size_t i = 0; i < 5; ++ i) {
    const int x = graphics.width() / 2 - 500 + 200 * i;
    const int y = 50 + 25 * std::sin((counter_ + i * M_PI) * 4 * M_PI);
    title_.draw(graphics, i, x, y);
  }

  if ((int)(counter_ * 2) % 2 == 0) {
    text_.draw(graphics, "Press any key", graphics.width() / 2, graphics.height() - 100, Text::Alignment::Center);
  }
}

Screen* TitleScreen::next_screen() const {
  return new GameScreen;
}
