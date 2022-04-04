#include "dialog.h"

#include <sstream>

#include "config.h"

Dialog::Dialog() :
  text_("text.png", 16),
  message_(""), timer_(0), index_(0) {}

void Dialog::set_message(const std::string& message) {
  message_ = message;
  index_ = 0;
}

void Dialog::update(float t) {
  if (!done()) {
    timer_ += t;
    if (timer_ > kRate) {
      ++index_;
      timer_ -= kRate;
    }
  }
}

void Dialog::draw(Graphics& graphics) const {
  text_.draw(graphics, message_.substr(0, index_), kConfig.graphics.width / 2 - 30 * 16, kConfig.graphics.height / 2);
}
