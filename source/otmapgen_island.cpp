//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"
#include "otmapgen_island.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include <set>
#include <queue>

OTMapGeneratorIsland::OTMapGeneratorIsland() : OTMapGeneratorBase() {
}

OTMapGeneratorIsland::~OTMapGeneratorIsland() {
}

bool OTMapGeneratorIsland::generateIslandMap(Map* map, const IslandConfig& config, int width, int height, const std::string& seed) {
    if (!map) return false;
    
    seedRandom(seed);
    
    // Calculate center point
    int centerX = width / 2;
    int centerY = height / 2;
    
    // Generate height map for island shape
    std::vector<std::vector<double>> heightMap(height, std::vector<double>(width, 0.0));
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate distance from center
            double distance = getIslandDistance(x, y, centerX, centerY, config.island_size);
            
            // Apply falloff
            double falloff = applyIslandFalloff(distance, config.island_falloff);
            
            // Generate noise
            double noise = 0.0;
            double amplitude = 1.0;
            double frequency = config.noise_scale;
            
            for (int octave = 0; octave < config.noise_octaves; ++octave) {
                noise += noise_generator->noise(x * frequency, y * frequency) * amplitude;
                amplitude *= config.noise_persistence;
                frequency *= config.noise_lacunarity;
            }
            
            // Combine noise with island shape
            heightMap[y][x] = (noise * 0.5 + 0.5) * falloff;
        }
    }
    
    // Apply smoothing if requested
    if (config.smoothing_passes > 0) {
        for (int pass = 0; pass < config.smoothing_passes; ++pass) {
            smoothHeightMap(heightMap, width, height);
        }
    }
    
    // Generate the actual map tiles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            double height_value = heightMap[y][x];
            
            Position pos(x, y, config.target_floor);
            Tile* tile = map->getTile(pos);
            if (!tile) {
                tile = map->allocator(map->createTileL(pos));
                map->setTile(pos, tile);
            }
            
            // Determine tile type based on height
            uint16_t tileId;
            if (height_value < config.water_level) {
                tileId = config.water_id;
            } else {
                tileId = config.ground_id;
            }
            
            // Set ground tile
            if (tile->ground) {
                delete tile->ground;
                tile->ground = nullptr;
            }
            
            Item* ground = Item::Create(tileId);
            if (ground) {
                tile->ground = ground;
            }
        }
    }
    
    // Post-processing cleanup if enabled
    if (config.enable_cleanup) {
        cleanupIsland(map, config, width, height);
    }
    
    return true;
}

double OTMapGeneratorIsland::getIslandDistance(int x, int y, int centerX, int centerY, double island_size) {
    double dx = static_cast<double>(x - centerX);
    double dy = static_cast<double>(y - centerY);
    return sqrt(dx * dx + dy * dy) / island_size;
}

double OTMapGeneratorIsland::applyIslandFalloff(double distance, double falloff) {
    if (distance >= 1.0) return 0.0;
    if (distance <= 0.0) return 1.0;
    
    return pow(1.0 - distance, falloff);
}

void OTMapGeneratorIsland::smoothHeightMap(std::vector<std::vector<double>>& heightMap, int width, int height) {
    std::vector<std::vector<double>> smoothed = heightMap;
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            double sum = 0.0;
            int count = 0;
            
            // Average with neighbors
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    sum += heightMap[y + dy][x + dx];
                    count++;
                }
            }
            
            smoothed[y][x] = sum / count;
        }
    }
    
    heightMap = smoothed;
}

void OTMapGeneratorIsland::cleanupIsland(Map* map, const IslandConfig& config, int width, int height) {
    // Remove small land patches
    if (config.min_land_patch_size > 0) {
        removeSmallLandPatches(map, config, width, height);
    }
    
    // Fill small water holes
    if (config.max_water_hole_size > 0) {
        fillSmallWaterHoles(map, config, width, height);
    }
}

void OTMapGeneratorIsland::removeSmallLandPatches(Map* map, const IslandConfig& config, int width, int height) {
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!visited[y][x]) {
                Position pos(x, y, config.target_floor);
                Tile* tile = map->getTile(pos);
                
                if (tile && tile->ground && tile->ground->getID() == config.ground_id) {
                    // Found unvisited land tile, flood fill to find patch size
                    std::vector<std::pair<int, int>> patch;
                    floodFillLand(map, config, x, y, width, height, visited, patch);
                    
                    // If patch is too small, convert to water
                    if (static_cast<int>(patch.size()) < config.min_land_patch_size) {
                        for (const auto& point : patch) {
                            Position patchPos(point.first, point.second, config.target_floor);
                            Tile* patchTile = map->getTile(patchPos);
                            if (patchTile && patchTile->ground) {
                                delete patchTile->ground;
                                patchTile->ground = Item::Create(config.water_id);
                            }
                        }
                    }
                }
            }
        }
    }
}

void OTMapGeneratorIsland::fillSmallWaterHoles(Map* map, const IslandConfig& config, int width, int height) {
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!visited[y][x]) {
                Position pos(x, y, config.target_floor);
                Tile* tile = map->getTile(pos);
                
                if (tile && tile->ground && tile->ground->getID() == config.water_id) {
                    // Found unvisited water tile, flood fill to find hole size
                    std::vector<std::pair<int, int>> hole;
                    floodFillWater(map, config, x, y, width, height, visited, hole);
                    
                    // If hole is too small, convert to land
                    if (static_cast<int>(hole.size()) <= config.max_water_hole_size) {
                        for (const auto& point : hole) {
                            Position holePos(point.first, point.second, config.target_floor);
                            Tile* holeTile = map->getTile(holePos);
                            if (holeTile && holeTile->ground) {
                                delete holeTile->ground;
                                holeTile->ground = Item::Create(config.ground_id);
                            }
                        }
                    }
                }
            }
        }
    }
}

void OTMapGeneratorIsland::floodFillLand(Map* map, const IslandConfig& config, int startX, int startY, 
                                       int width, int height, std::vector<std::vector<bool>>& visited, 
                                       std::vector<std::pair<int, int>>& patch) {
    std::queue<std::pair<int, int>> queue;
    queue.push({startX, startY});
    visited[startY][startX] = true;
    
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};
    
    while (!queue.empty()) {
        auto [x, y] = queue.front();
        queue.pop();
        patch.push_back({x, y});
        
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[ny][nx]) {
                Position pos(nx, ny, config.target_floor);
                Tile* tile = map->getTile(pos);
                
                if (tile && tile->ground && tile->ground->getID() == config.ground_id) {
                    visited[ny][nx] = true;
                    queue.push({nx, ny});
                }
            }
        }
    }
}

void OTMapGeneratorIsland::floodFillWater(Map* map, const IslandConfig& config, int startX, int startY, 
                                        int width, int height, std::vector<std::vector<bool>>& visited, 
                                        std::vector<std::pair<int, int>>& hole) {
    std::queue<std::pair<int, int>> queue;
    queue.push({startX, startY});
    visited[startY][startX] = true;
    
    const int dx[] = {-1, 1, 0, 0};
    const int dy[] = {0, 0, -1, 1};
    
    while (!queue.empty()) {
        auto [x, y] = queue.front();
        queue.pop();
        hole.push_back({x, y});
        
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[ny][nx]) {
                Position pos(nx, ny, config.target_floor);
                Tile* tile = map->getTile(pos);
                
                if (tile && tile->ground && tile->ground->getID() == config.water_id) {
                    visited[ny][nx] = true;
                    queue.push({nx, ny});
                }
            }
        }
    }
} 