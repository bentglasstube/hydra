#include "config.h"

Config::Config() : Game::Config() {
  graphics.title = "Hydra";
  graphics.width = 1280;
  graphics.height = 720;
  graphics.intscale = false;
  graphics.fullscreen = false;
}
