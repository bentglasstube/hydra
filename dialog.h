#pragma once

#include <string>

#include "text.h"

class Dialog {
  public:

    Dialog();

    void set_message(const std::string& message);
    void update(float t);
    void draw(Graphics& graphics) const;
    bool done() const { return index_ >= message_.length(); }
    void dismiss() { message_ = ""; }

    operator bool() const { return message_.length() > 0; }

  private:

    static constexpr float kRate = 0.075f;

    Text text_;
    std::string message_;
    float  timer_;
    size_t index_;
};
