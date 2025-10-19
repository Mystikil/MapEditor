#include "Rivers.h"

#include "MapGenerator.h"

#include "item.h"
#include "map.h"
#include "position.h"
#include "tile.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Rivers {
namespace {

Tile* ensureTile(Map& map, int x, int y, int z) {
        Tile* tile = map.getTile(x, y, z);
        if (!tile) {
                tile = map.allocator.allocateTile(map.createTileL(x, y, z));
                map.setTile(x, y, z, tile, true);
        }
        return tile;
}

void setWaterTile(Tile* tile, uint16_t waterId) {
        if (tile->ground) {
                delete tile->ground;
                tile->ground = nullptr;
        }
        for (Item* item : tile->items) {
                delete item;
        }
        tile->items.clear();
        tile->addItem(Item::Create(waterId));
}

} // namespace

void Generate(Map& map,
              const Biomes::BiomePreset& preset,
              const GenOptions& options,
              const std::vector<double>& height,
              std::mt19937_64& rng) {
        if (options.width <= 0 || options.height <= 0) {
                return;
        }

        struct Source {
                double h;
                int x;
                int y;
        };

        std::vector<Source> sources;
        sources.reserve(static_cast<size_t>(options.width) * 2);
        for (int y = 1; y < options.height - 1; ++y) {
                for (int x = 1; x < options.width - 1; ++x) {
                        const size_t index = static_cast<size_t>(y) * static_cast<size_t>(options.width) + static_cast<size_t>(x);
                        const double h = height[index];
                        if (h > preset.mountainLevel && h > preset.seaLevel + 0.1) {
                                sources.push_back({h, x, y});
                        }
                }
        }

        if (sources.empty()) {
                return;
        }

        std::sort(sources.begin(), sources.end(), [](const Source& a, const Source& b) {
                return a.h > b.h;
        });

        const int riverCount = std::max(1, static_cast<int>((options.width * options.height) / 262144) + 1);
        std::vector<int> flow(options.width * options.height, 0);

        std::uniform_int_distribution<size_t> pickSource(0, std::min<size_t>(sources.size() - 1, static_cast<size_t>(riverCount * 2)));

        const int dirs[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};

        for (int river = 0; river < riverCount; ++river) {
                const Source& source = sources[pickSource(rng)];
                int x = source.x;
                int y = source.y;
                double lastHeight = source.h;

                for (int step = 0; step < options.width + options.height; ++step) {
                        if (x < 0 || y < 0 || x >= options.width || y >= options.height) {
                                break;
                        }
                        const size_t index = static_cast<size_t>(y) * static_cast<size_t>(options.width) + static_cast<size_t>(x);
                        ++flow[index];
                        Tile* tile = ensureTile(map, x, y, options.overworldZ);
                        setWaterTile(tile, height[index] < preset.seaLevel ? preset.tiles.deepWater : preset.tiles.shallowWater);

                        // Widen the river slightly as flow accumulates.
                        if (flow[index] > 2) {
                                for (const auto& d : dirs) {
                                        const int nx = x + d[0];
                                        const int ny = y + d[1];
                                        if (nx < 0 || ny < 0 || nx >= options.width || ny >= options.height) {
                                                continue;
                                        }
                                        Tile* shore = ensureTile(map, nx, ny, options.overworldZ);
                                        if (shore->ground && shore->ground->getID() == preset.tiles.shallowWater) {
                                                continue;
                                        }
                                        if (height[index] < preset.seaLevel) {
                                                setWaterTile(shore, preset.tiles.deepWater);
                                        } else if (flow[index] > 4) {
                                                // Add a bit of shoreline on heavy flow
                                                if (shore->ground) {
                                                        delete shore->ground;
                                                        shore->ground = nullptr;
                                                }
                                                for (Item* item : shore->items) {
                                                        delete item;
                                                }
                                                shore->items.clear();
                                                shore->addItem(Item::Create(preset.tiles.sand));
                                        }
                                }
                        }

                        if (height[index] < preset.seaLevel) {
                                break; // reached sea or lake
                        }

                        double bestHeight = height[index];
                        int bestDx = 0;
                        int bestDy = 0;
                        for (const auto& dir : dirs) {
                                const int nx = x + dir[0];
                                const int ny = y + dir[1];
                                if (nx < 0 || ny < 0 || nx >= options.width || ny >= options.height) {
                                        continue;
                                }
                                const size_t nIndex = static_cast<size_t>(ny) * static_cast<size_t>(options.width) + static_cast<size_t>(nx);
                                const double nh = height[nIndex];
                                if (nh < bestHeight - 0.0005) {
                                        bestHeight = nh;
                                        bestDx = dir[0];
                                        bestDy = dir[1];
                                }
                        }

                        if (bestDx == 0 && bestDy == 0) {
                                // Could not find a lower neighbor, gently nudge towards the lowest one available
                                double lowest = lastHeight;
                                for (const auto& dir : dirs) {
                                        const int nx = x + dir[0];
                                        const int ny = y + dir[1];
                                        if (nx < 0 || ny < 0 || nx >= options.width || ny >= options.height) {
                                                continue;
                                        }
                                        const size_t nIndex = static_cast<size_t>(ny) * static_cast<size_t>(options.width) + static_cast<size_t>(nx);
                                        const double nh = height[nIndex];
                                        if (nh < lowest) {
                                                lowest = nh;
                                                bestDx = dir[0];
                                                bestDy = dir[1];
                                        }
                                }
                                if (bestDx == 0 && bestDy == 0) {
                                        break;
                                }
                        }

                        x += bestDx;
                        y += bestDy;
                        lastHeight = bestHeight;
                }
        }
}

} // namespace Rivers

