#include "title_screen.h"

TitleScreen::TitleScreen() : text_("text.png") {}

bool TitleScreen::update(const Input& input, Audio&, unsigned int) {
  return !input.any_pressed();
}

void TitleScreen::draw(Graphics& graphics) const {
  text_.draw(graphics, "Press any key", graphics.width() / 2, graphics.height() - 32, Text::Alignment::Center);
}

Screen* TitleScreen::next_screen() const {
  return nullptr;
}
