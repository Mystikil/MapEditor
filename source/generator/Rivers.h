#pragma once

#include <random>
#include <vector>

#include "Biomes.h"

class Map;
struct GenOptions;

namespace Rivers {

void Generate(Map& map,
              const Biomes::BiomePreset& preset,
              const GenOptions& options,
              const std::vector<double>& height,
              std::mt19937_64& rng);

}

