#include "MapGenerator.h"

#include "Biomes.h"
#include "CaveCarver.h"
#include "Dungeons.h"
#include "Noise.h"
#include "Rivers.h"

#include "gui.h"
#include "item.h"
#include "map.h"
#include "position.h"
#include "tile.h"
#include "town.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <wx/log.h>

namespace {

struct NoiseFields {
        std::vector<double> height;
        std::vector<double> temperature;
        std::vector<double> moisture;
};

struct GenerationCancelled : public std::runtime_error {
        GenerationCancelled() : std::runtime_error("generation_cancelled") {}
};

size_t indexAt(int x, int y, int width) {
        return static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
}

NoiseFields buildNoiseFields(const GenOptions& opt) {
        Noise noise(opt.seed);
        NoiseFields fields;
        const size_t total = static_cast<size_t>(opt.width) * static_cast<size_t>(opt.height);
        fields.height.resize(total);
        fields.temperature.resize(total);
        fields.moisture.resize(total);

        const double invWidth = 1.0 / std::max(1, opt.width - 1);
        const double invHeight = 1.0 / std::max(1, opt.height - 1);

        for (int y = 0; y < opt.height; ++y) {
                for (int x = 0; x < opt.width; ++x) {
                        const double nx = static_cast<double>(x) * invWidth;
                        const double ny = static_cast<double>(y) * invHeight;

                        double heightValue = noise.fractal(nx * 1.2, ny * 1.2, 5, 1.0, 0.5, 2.0);
                        heightValue = std::clamp(heightValue * 0.5 + 0.5, 0.0, 1.0);

                        // Apply a mild radial falloff so edges trend towards sea level.
                        const double dx = nx - 0.5;
                        const double dy = ny - 0.5;
                        const double distance = std::sqrt(dx * dx + dy * dy);
                        const double falloff = std::clamp(1.0 - distance * 0.9, 0.0, 1.0);

                        fields.height[indexAt(x, y, opt.width)] = std::clamp(heightValue * falloff + 0.1 * heightValue, 0.0, 1.0);

                        double temperatureValue = noise.fractal(nx * 0.8 + 42.0, ny * 0.8 - 17.0, 4, 1.0, 0.6, 2.0);
                        temperatureValue = std::clamp(temperatureValue * 0.5 + 0.5, 0.0, 1.0);

                        // Make the north (top) colder and south warmer.
                        temperatureValue = std::clamp(temperatureValue * 0.7 + (1.0 - ny) * 0.3, 0.0, 1.0);
                        fields.temperature[indexAt(x, y, opt.width)] = temperatureValue;

                        double moistureValue = noise.fractal(nx * 1.6 - 11.0, ny * 1.6 + 7.0, 4, 1.0, 0.55, 2.1);
                        moistureValue = std::clamp(moistureValue * 0.5 + 0.5, 0.0, 1.0);
                        fields.moisture[indexAt(x, y, opt.width)] = moistureValue;
                }
        }

        return fields;
}

Tile* ensureTile(Map& map, int x, int y, int z) {
        Tile* tile = map.getTile(x, y, z);
        if (!tile) {
                tile = map.allocator.allocateTile(map.createTileL(x, y, z));
                map.setTile(x, y, z, tile, true);
        }
        return tile;
}

void clearTileItems(Tile* tile) {
        if (!tile) {
                return;
        }
        if (tile->ground) {
                delete tile->ground;
                tile->ground = nullptr;
        }
        for (Item* item : tile->items) {
                delete item;
        }
        tile->items.clear();
}

void placeStarterTown(Map& map, const GenOptions& opt, const Biomes::BiomePreset& preset) {
        if (!preset.spawn.enabled) {
                return;
        }

        const int centerX = opt.width / 2;
        const int centerY = opt.height / 2;
        const int radius = std::max(2, preset.spawn.radius);
        for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                        const int x = centerX + dx;
                        const int y = centerY + dy;
                        if (x < 0 || y < 0 || x >= opt.width || y >= opt.height) {
                                continue;
                        }
                        Tile* tile = ensureTile(map, x, y, opt.overworldZ);
                        clearTileItems(tile);
                        tile->addItem(Item::Create(preset.tiles.dirt));
                }
        }

        Tile* templeTile = ensureTile(map, centerX, centerY, opt.overworldZ);
        templeTile->addItem(Item::Create(preset.spawn.templeTile));

        map.towns.clear();
        Town* starterTown = newd Town(1);
        starterTown->setName("Starter Town");
        starterTown->setTemplePosition(Position(centerX, centerY, opt.overworldZ));
        map.towns.addTown(starterTown);
}

} // namespace

std::unique_ptr<Map> MapGenerator::Generate(const GenOptions& opt, std::function<bool(int, int)> progress) {
        if (opt.width <= 0 || opt.height <= 0) {
                throw std::runtime_error("Map dimensions must be positive");
        }
        if (opt.chunkSize <= 0) {
                throw std::runtime_error("Chunk size must be positive");
        }

        const std::string dataDir = GUI::GetDataDirectory().ToStdString();
        const Biomes::BiomePreset preset = Biomes::LoadPreset(dataDir, opt.biomePreset);

        auto generatedMap = std::make_unique<Map>();
        generatedMap->setWidth(opt.width);
        generatedMap->setHeight(opt.height);
        generatedMap->setMapDescription("Procedurally generated map");
        generatedMap->setName("Generated-" + opt.biomePreset + "-" + std::to_string(opt.seed) + ".otbm");
        generatedMap->setSpawnFilename("generated-spawn.xml");
        generatedMap->setHouseFilename("generated-house.xml");

        const NoiseFields fields = buildNoiseFields(opt);

        const int chunkSize = std::max(32, opt.chunkSize);
        const int chunksX = (opt.width + chunkSize - 1) / chunkSize;
        const int chunksY = (opt.height + chunkSize - 1) / chunkSize;

        int totalSteps = chunksX * chunksY;
        if (opt.makeRivers) {
                ++totalSteps;
        }
        if (opt.makeCaves) {
                ++totalSteps;
        }
        if (opt.makeDungeons) {
                ++totalSteps;
        }
        if (opt.placeStarterTown) {
                ++totalSteps;
        }

        std::mt19937_64 rng(opt.seed);

        int completed = 0;
        auto advance = [&](const std::string& stageLabel) {
                ++completed;
                if (progress && !progress(completed, totalSteps)) {
                        throw GenerationCancelled();
                }
                if (!stageLabel.empty()) {
                        wxLogMessage("[Generator] %s (%d/%d)", stageLabel, completed, totalSteps);
                }
        };

        for (int cy = 0; cy < chunksY; ++cy) {
                for (int cx = 0; cx < chunksX; ++cx) {
                        const int startX = cx * chunkSize;
                        const int startY = cy * chunkSize;
                        const int endX = std::min(opt.width, startX + chunkSize);
                        const int endY = std::min(opt.height, startY + chunkSize);

                        Biomes::ApplyTerrainChunk(
                                *generatedMap,
                                preset,
                                opt,
                                fields.height,
                                fields.temperature,
                                fields.moisture,
                                startX,
                                startY,
                                endX,
                                endY,
                                rng
                        );

                        advance("Terrain");
                }
        }

        if (opt.makeRivers) {
                Rivers::Generate(*generatedMap, preset, opt, fields.height, rng);
                advance("Rivers");
        }

        if (opt.makeCaves) {
                CaveCarver::Generate(*generatedMap, preset, opt, fields.height, rng);
                advance("Caves");
        }

        if (opt.makeDungeons) {
                Dungeons::Generate(*generatedMap, preset, opt, rng);
                advance("Dungeons");
        }

        if (opt.placeStarterTown) {
                placeStarterTown(*generatedMap, opt, preset);
                advance("Starter town");
        }

        generatedMap->doChange();
        wxLogMessage("Generated map seed=%llu preset=%s size=%dx%d", static_cast<unsigned long long>(opt.seed), opt.biomePreset, opt.width, opt.height);

        return generatedMap;
}

