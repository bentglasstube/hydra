#include "config.h"

Config::Config() : Game::Config() {
  graphics.title = "Hydra";
  graphics.width = 1920;
  graphics.height = 1080;
  graphics.intscale = false;
  graphics.fullscreen = false;
}
