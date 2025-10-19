#pragma once

#include <random>

#include "Biomes.h"

class Map;
struct GenOptions;

namespace Dungeons {

void Generate(Map& map,
              const Biomes::BiomePreset& preset,
              const GenOptions& options,
              std::mt19937_64& rng);

}

