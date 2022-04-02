#pragma once

#include "game.h"

struct Config : public Game::Config { Config(); };
static const Config kConfig;
