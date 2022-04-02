#include "game.h"

#include "config.h"
#include "title_screen.h"

#ifdef __EMSCRIPTEN__
#include "emscripten.h"

void step(void* game) {
  static_cast<Game*>(game)->step();
}
#endif

int main(int, char**) {
  Game game(kConfig);
  Screen *start = new TitleScreen();

#ifdef __EMSCRIPTEN__
  game.start(start);
  emscripten_set_main_loop_arg(step, &game, 0, true);
#else
  game.loop(start);
#endif

  return 0;
}
