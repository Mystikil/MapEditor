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
#include "otmapgen_procedural.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include <algorithm>

OTMapGeneratorProcedural::OTMapGeneratorProcedural() : OTMapGeneratorBase() {
}

OTMapGeneratorProcedural::~OTMapGeneratorProcedural() {
}

bool OTMapGeneratorProcedural::generateProceduralMap(Map* map, const ProceduralConfig& config) {
    if (!map) return false;
    
    seedRandom(config.seed);
    
    // Generate multiple floors if requested
    if (config.floors_to_generate > 1) {
        generateMultipleFloors(map, config);
    } else {
        generateFloor(map, config, config.target_floor);
    }
    
    return true;
}

void OTMapGeneratorProcedural::generateMultipleFloors(Map* map, const ProceduralConfig& config) {
    for (int floor = 0; floor < config.floors_to_generate; ++floor) {
        int targetFloor = config.target_floor + floor;
        generateFloor(map, config, targetFloor);
    }
}

void OTMapGeneratorProcedural::generateFloor(Map* map, const ProceduralConfig& config, int floor) {
    // Generate height and moisture maps
    std::vector<std::vector<double>> heightMap = generateNoiseMap(config);
    std::vector<std::vector<double>> moistureMap = generateMoistureMap(config);
    
    // Apply island mask if needed
    if (config.island_distance_exponent > 0.0) {
        heightMap = applyIslandMask(heightMap, config);
    }
    
    // Generate terrain based on height and moisture
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double height = heightMap[y][x];
            double moisture = moistureMap[y][x];
            
            Position pos(x, y, floor);
            Tile* tile = map->getTile(pos);
            if (!tile) {
                tile = map->allocator(map->createTileL(pos));
                map->setTile(pos, tile);
            }
            
            // Select appropriate terrain
            uint16_t tileId = getTerrainTileId(height, moisture, config);
            
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
    
    // Generate caves if enabled
    if (config.add_caves) {
        generateCaves(map, config, floor);
    }
    
    // Apply post-processing
    if (!config.terrain_only) {
        applyTerrainSmoothing(map, config, floor);
        addTerrainTransitions(map, config, floor);
    }
}

std::vector<std::vector<double>> OTMapGeneratorProcedural::generateNoiseMap(const ProceduralConfig& config) {
    std::vector<std::vector<double>> heightMap(config.height, std::vector<double>(config.width, 0.0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double noise = 0.0;
            
            if (!config.frequencies.empty()) {
                // Use frequency weights if available
                noise = noise_generator->fractal(x * config.noise_increment, y * config.noise_increment, config.frequencies);
            } else {
                // Use basic noise
                noise = noise_generator->noise(x * config.noise_increment, y * config.noise_increment);
            }
            
            // Apply transformations
            noise = pow(noise * config.linear_multiplier, config.exponent);
            
            // Normalize to [0, 1]
            heightMap[y][x] = std::max(0.0, std::min(1.0, (noise + 1.0) * 0.5));
        }
    }
    
    return heightMap;
}

std::vector<std::vector<double>> OTMapGeneratorProcedural::generateMoistureMap(const ProceduralConfig& config) {
    std::vector<std::vector<double>> moistureMap(config.height, std::vector<double>(config.width, 0.0));
    
    // Use different offset for moisture to create variation
    double moistureOffset = 1000.0;
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double moisture = noise_generator->noise(
                (x + moistureOffset) * config.noise_increment * 0.5,
                (y + moistureOffset) * config.noise_increment * 0.5
            );
            
            // Normalize to [-1, 1] for moisture (can be negative for dry areas)
            moistureMap[y][x] = moisture;
        }
    }
    
    return moistureMap;
}

std::vector<std::vector<double>> OTMapGeneratorProcedural::applyIslandMask(const std::vector<std::vector<double>>& heightMap, const ProceduralConfig& config) {
    std::vector<std::vector<double>> maskedMap = heightMap;
    
    int centerX = config.width / 2;
    int centerY = config.height / 2;
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double distance = calculateIslandDistance(x - centerX, y - centerY, config);
            double falloff = applyDistanceFalloff(distance, config);
            
            maskedMap[y][x] = heightMap[y][x] * falloff;
        }
    }
    
    return maskedMap;
}

double OTMapGeneratorProcedural::calculateIslandDistance(int x, int y, const ProceduralConfig& config) {
    if (config.euclidean_distance) {
        return sqrt(x * x + y * y);
    } else {
        return abs(x) + abs(y); // Manhattan distance
    }
}

double OTMapGeneratorProcedural::applyDistanceFalloff(double distance, const ProceduralConfig& config) {
    double normalizedDistance = distance * config.island_distance_decrement;
    double falloff = 1.0 - pow(normalizedDistance, config.island_distance_exponent);
    return std::max(0.0, falloff);
}

uint16_t OTMapGeneratorProcedural::getTerrainTileId(double height, double moisture, const ProceduralConfig& config) {
    // Check if it should be water
    if (height < (config.water_level / 100.0)) {
        return config.water_id;
    }
    
    // Find best matching terrain layer
    const TerrainLayer* layer = selectBestTerrainLayer(height, moisture, config);
    
    if (layer && layer->enabled) {
        return layer->item_id;
    }
    
    // Fallback to basic grass or stone
    return 4526; // Default grass tile
}

const TerrainLayer* OTMapGeneratorProcedural::selectBestTerrainLayer(double height, double moisture, const ProceduralConfig& config) {
    const TerrainLayer* bestLayer = nullptr;
    int bestScore = -1;
    
    for (const auto& layer : config.terrain_layers) {
        if (!layer.enabled) continue;
        
        // Check if height and moisture are within range
        if (isValidTerrainPlacement(layer, height, moisture)) {
            // Calculate score based on how well it fits
            int score = layer.z_order;
            
            // Prefer layers that better match the center of their range
            double heightCenter = (layer.height_min + layer.height_max) * 0.5;
            double moistureCenter = (layer.moisture_min + layer.moisture_max) * 0.5;
            
            double heightDistance = abs(height - heightCenter);
            double moistureDistance = abs(moisture - moistureCenter);
            
            // Lower distance = higher score
            score += static_cast<int>((1.0 - heightDistance - moistureDistance) * 100);
            
            if (score > bestScore) {
                bestScore = score;
                bestLayer = &layer;
            }
        }
    }
    
    return bestLayer;
}

bool OTMapGeneratorProcedural::isValidTerrainPlacement(const TerrainLayer& layer, double height, double moisture) {
    return height >= layer.height_min && height <= layer.height_max &&
           moisture >= layer.moisture_min && moisture <= layer.moisture_max;
}

void OTMapGeneratorProcedural::generateCaves(Map* map, const ProceduralConfig& config, int floor) {
    // Generate caves on floors below the surface
    if (floor <= config.target_floor) return;
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double caveNoise = noise_generator->noise(
                x * config.cave_roughness,
                y * config.cave_roughness
            );
            
            // Calculate height for cave probability
            double surfaceHeight = noise_generator->noise(
                x * config.noise_increment,
                y * config.noise_increment
            );
            
            if (shouldPlaceCave(surfaceHeight, caveNoise, config)) {
                placeCaveTile(map, x, y, floor, config);
            }
        }
    }
}

bool OTMapGeneratorProcedural::shouldPlaceCave(double height, double noise, const ProceduralConfig& config) {
    // Higher chance of caves in mountainous areas
    double caveThreshold = config.cave_chance + (height * 0.3);
    return (noise + 1.0) * 0.5 > caveThreshold;
}

void OTMapGeneratorProcedural::placeCaveTile(Map* map, int x, int y, int z, const ProceduralConfig& config) {
    Position pos(x, y, z);
    Tile* tile = map->getTile(pos);
    if (!tile) {
        tile = map->allocator(map->createTileL(pos));
        map->setTile(pos, tile);
    }
    
    // Remove existing ground and replace with cave wall
    if (tile->ground) {
        delete tile->ground;
        tile->ground = nullptr;
    }
    
    Item* caveWall = Item::Create(config.cave_wall_id);
    if (caveWall) {
        tile->ground = caveWall;
    }
}

void OTMapGeneratorProcedural::applyTerrainSmoothing(Map* map, const ProceduralConfig& config, int floor) {
    // Simple smoothing pass to reduce harsh transitions
    std::vector<std::vector<uint16_t>> tileIds(config.height, std::vector<uint16_t>(config.width, 0));
    
    // Collect current tile IDs
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            Position pos(x, y, floor);
            Tile* tile = map->getTile(pos);
            if (tile && tile->ground) {
                tileIds[y][x] = tile->ground->getID();
            }
        }
    }
    
    // Apply smoothing
    for (int y = 1; y < config.height - 1; ++y) {
        for (int x = 1; x < config.width - 1; ++x) {
            // Count neighbor tile types
            std::map<uint16_t, int> neighborCount;
            
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    uint16_t neighborId = tileIds[y + dy][x + dx];
                    neighborCount[neighborId]++;
                }
            }
            
            // Find most common neighbor
            uint16_t mostCommon = tileIds[y][x];
            int maxCount = 0;
            
            for (const auto& pair : neighborCount) {
                if (pair.second > maxCount) {
                    maxCount = pair.second;
                    mostCommon = pair.first;
                }
            }
            
            // Replace if significantly different
            if (maxCount >= 6 && mostCommon != tileIds[y][x]) {
                Position pos(x, y, floor);
                Tile* tile = map->getTile(pos);
                if (tile && tile->ground) {
                    delete tile->ground;
                    tile->ground = Item::Create(mostCommon);
                }
            }
        }
    }
}

void OTMapGeneratorProcedural::addTerrainTransitions(Map* map, const ProceduralConfig& config, int floor) {
    // Add natural transitions between different terrain types
    // This is a placeholder for more complex terrain blending
    // For now, we'll just ensure water edges are properly handled
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            Position pos(x, y, floor);
            Tile* tile = map->getTile(pos);
            
            if (tile && tile->ground && tile->ground->getID() == config.water_id) {
                // Check if this water tile is adjacent to land
                bool hasLandNeighbor = false;
                
                for (int dy = -1; dy <= 1 && !hasLandNeighbor; ++dy) {
                    for (int dx = -1; dx <= 1 && !hasLandNeighbor; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        
                        int nx = x + dx;
                        int ny = y + dy;
                        
                        if (nx >= 0 && nx < config.width && ny >= 0 && ny < config.height) {
                            Position neighborPos(nx, ny, floor);
                            Tile* neighborTile = map->getTile(neighborPos);
                            
                            if (neighborTile && neighborTile->ground && 
                                neighborTile->ground->getID() != config.water_id) {
                                hasLandNeighbor = true;
                            }
                        }
                    }
                }
                
                // This water tile is at the shore - could add special shore tiles here
                if (hasLandNeighbor) {
                    // For now, keep as water but could be enhanced with shore tiles
                }
            }
        }
    }
} 