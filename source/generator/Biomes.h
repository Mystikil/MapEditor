#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

class Map;
struct GenOptions;

namespace Biomes {

struct DecorEntry {
        std::string name;
        uint16_t tileId = 0;
        double density = 0.0;
};

struct BiomeTiles {
        uint16_t deepWater = 0;
        uint16_t shallowWater = 0;
        uint16_t sand = 0;
        uint16_t grass = 0;
        uint16_t rock = 0;
        uint16_t snow = 0;
        uint16_t dirt = 0;
};

struct SpawnSettings {
        bool enabled = false;
        uint16_t templeTile = 0;
        int radius = 0;
};

struct BiomePreset {
        double seaLevel = 0.45;
        double mountainLevel = 0.8;
        BiomeTiles tiles;
        std::vector<DecorEntry> decor;
        SpawnSettings spawn;
};

std::vector<std::string> ListPresets(const std::string& dataDirectory);
BiomePreset LoadPreset(const std::string& dataDirectory, const std::string& name);

void ApplyTerrainChunk(
        Map& map,
        const BiomePreset& preset,
        const GenOptions& options,
        const std::vector<double>& height,
        const std::vector<double>& temperature,
        const std::vector<double>& moisture,
        int startX,
        int startY,
        int endX,
        int endY,
        std::mt19937_64& rng
);

} // namespace Biomes

