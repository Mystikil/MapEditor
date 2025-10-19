#include "CaveCarver.h"

#include "MapGenerator.h"

#include "item.h"
#include "map.h"
#include "tile.h"

#include <algorithm>
#include <random>
#include <vector>

namespace CaveCarver {
namespace {

Tile* ensureTile(Map& map, int x, int y, int z) {
        Tile* tile = map.getTile(x, y, z);
        if (!tile) {
                tile = map.allocator.allocateTile(map.createTileL(x, y, z));
                map.setTile(x, y, z, tile, true);
        }
        return tile;
}

void setGround(Tile* tile, uint16_t id) {
        if (tile->ground) {
                delete tile->ground;
                tile->ground = nullptr;
        }
        for (Item* item : tile->items) {
                delete item;
        }
        tile->items.clear();
        tile->addItem(Item::Create(id));
}

int countWalls(const std::vector<uint8_t>& grid, int width, int height, int x, int y) {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) {
                                continue;
                        }
                        const int nx = x + dx;
                        const int ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                                ++count;
                        } else {
                                count += grid[ny * width + nx] ? 1 : 0;
                        }
                }
        }
        return count;
}

} // namespace

void Generate(Map& map,
              const Biomes::BiomePreset& preset,
              const GenOptions& options,
              const std::vector<double>& heightField,
              std::mt19937_64& rng) {
        if (options.width <= 0 || options.height <= 0) {
                return;
        }

        const int width = options.width;
        const int height = options.height;

        std::vector<uint8_t> cells(static_cast<size_t>(width) * height, 0);
        std::bernoulli_distribution initialChance(0.45);
        for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                        cells[static_cast<size_t>(y) * width + x] = initialChance(rng) ? 1 : 0;
                }
        }

        std::vector<uint8_t> scratch(cells.size(), 0);
        for (int step = 0; step < 5; ++step) {
                for (int y = 0; y < height; ++y) {
                        for (int x = 0; x < width; ++x) {
                                const int wallCount = countWalls(cells, width, height, x, y);
                                const size_t idx = static_cast<size_t>(y) * width + x;
                                if (wallCount > 4) {
                                        scratch[idx] = 1;
                                } else if (wallCount < 4) {
                                        scratch[idx] = 0;
                                } else {
                                        scratch[idx] = cells[idx];
                                }
                        }
                }
                cells.swap(scratch);
        }

        std::vector<std::pair<int, int>> entrances;
        entrances.reserve(4);

        for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                        const size_t idx = static_cast<size_t>(y) * width + x;
                        Tile* caveTile = ensureTile(map, x, y, options.caveZ);
                        if (cells[idx] == 1) {
                                setGround(caveTile, preset.tiles.rock);
                        } else {
                                setGround(caveTile, preset.tiles.dirt);
                                if (entrances.size() < 8 && heightField[idx] > preset.seaLevel + 0.05) {
                                        entrances.emplace_back(x, y);
                                }
                        }
                }
        }

        if (!entrances.empty()) {
                std::shuffle(entrances.begin(), entrances.end(), rng);
                const size_t count = std::min<size_t>(entrances.size(), 3);
                for (size_t i = 0; i < count; ++i) {
                        const int x = entrances[i].first;
                        const int y = entrances[i].second;
                        Tile* surface = ensureTile(map, x, y, options.overworldZ);
                        setGround(surface, preset.tiles.dirt);
                        Tile* cave = ensureTile(map, x, y, options.caveZ);
                        setGround(cave, preset.tiles.dirt);
                }
        }
}

} // namespace CaveCarver

