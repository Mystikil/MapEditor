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
#include "otmapgen.h"
#include "basemap.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include "map.h"

#include <cmath>
#include <algorithm>
#include <functional>
#include <iostream>

// SimplexNoise implementation
const double SimplexNoise::F2 = 0.5 * (sqrt(3.0) - 1.0);
const double SimplexNoise::G2 = (3.0 - sqrt(3.0)) / 6.0;

const int SimplexNoise::SIMPLEX[64][4] = {
    {0,1,2,3},{0,1,3,2},{0,0,0,0},{0,2,3,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,2,3,0},
    {0,2,1,3},{0,0,0,0},{0,3,1,2},{0,3,2,1},{0,0,0,0},{0,0,0,0},{0,0,0,0},{1,3,2,0},
    {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
    {1,2,0,3},{0,0,0,0},{1,3,0,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,3,0,1},{2,3,1,0},
    {1,0,2,3},{1,0,3,2},{0,0,0,0},{0,0,0,0},{0,0,0,0},{2,0,3,1},{0,0,0,0},{2,1,3,0},
    {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
    {2,0,1,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,0,1,2},{3,0,2,1},{0,0,0,0},{3,1,2,0},
    {2,1,0,3},{0,0,0,0},{0,0,0,0},{0,0,0,0},{3,1,0,2},{0,0,0,0},{3,2,0,1},{3,2,1,0}
};

SimplexNoise::SimplexNoise(unsigned int seed) {
    initializePermutation(seed);
}

void SimplexNoise::initializePermutation(unsigned int seed) {
    // Initialize permutation table based on seed
    std::mt19937 rng(seed);
    
    // Initialize with sequential values
    for (int i = 0; i < 256; i++) {
        perm[i] = i;
    }
    
    // Shuffle the permutation table
    for (int i = 255; i > 0; i--) {
        int j = rng() % (i + 1);
        std::swap(perm[i], perm[j]);
    }
    
    // Duplicate the permutation table
    for (int i = 0; i < 256; i++) {
        perm[256 + i] = perm[i];
        permMod12[i] = perm[i] % 12;
        permMod12[256 + i] = perm[i] % 12;
    }
}

int SimplexNoise::fastfloor(double x) {
    int xi = (int)x;
    return x < xi ? xi - 1 : xi;
}

double SimplexNoise::dot(const int g[], double x, double y) {
    return g[0] * x + g[1] * y;
}

double SimplexNoise::noise(double xin, double yin) {
    static const int grad3[12][3] = {
        {1,1,0},{-1,1,0},{1,-1,0},{-1,-1,0},
        {1,0,1},{-1,0,1},{1,0,-1},{-1,0,-1},
        {0,1,1},{0,-1,1},{0,1,-1},{0,-1,-1}
    };
    
    double n0, n1, n2; // Noise contributions from the three corners
    
    // Skew the input space to determine which simplex cell we're in
    double s = (xin + yin) * F2; // Hairy factor for 2D
    int i = fastfloor(xin + s);
    int j = fastfloor(yin + s);
    double t = (i + j) * G2;
    double X0 = i - t; // Unskew the cell origin back to (x,y) space
    double Y0 = j - t;
    double x0 = xin - X0; // The x,y distances from the cell origin
    double y0 = yin - Y0;
    
    // For the 2D case, the simplex shape is an equilateral triangle
    // Determine which simplex we are in
    int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
    if (x0 > y0) {
        i1 = 1; j1 = 0; // lower triangle, XY order: (0,0)->(1,0)->(1,1)
    } else {
        i1 = 0; j1 = 1; // upper triangle, YX order: (0,0)->(0,1)->(1,1)
    }
    
    // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
    // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
    // c = (3-sqrt(3))/6
    double x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
    double y1 = y0 - j1 + G2;
    double x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
    double y2 = y0 - 1.0 + 2.0 * G2;
    
    // Work out the hashed gradient indices of the three simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int gi0 = permMod12[ii + perm[jj]];
    int gi1 = permMod12[ii + i1 + perm[jj + j1]];
    int gi2 = permMod12[ii + 1 + perm[jj + 1]];
    
    // Calculate the contribution from the three corners
    double t0 = 0.5 - x0 * x0 - y0 * y0;
    if (t0 < 0) {
        n0 = 0.0;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad3[gi0], x0, y0); // (x,y) of grad3 used for 2D gradient
    }
    
    double t1 = 0.5 - x1 * x1 - y1 * y1;
    if (t1 < 0) {
        n1 = 0.0;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
    }
    
    double t2 = 0.5 - x2 * x2 - y2 * y2;
    if (t2 < 0) {
        n2 = 0.0;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
    }
    
    // Add contributions from each corner to get the final noise value
    // The result is scaled to return values in the interval [-1,1]
    return 70.0 * (n0 + n1 + n2);
}

double SimplexNoise::fractal(double x, double y, const std::vector<GenerationConfig::FrequencyWeight>& frequencies) {
    double value = 0.0;
    double totalWeight = 0.0;
    
    for (const auto& freq : frequencies) {
        value += noise(x * freq.frequency, y * freq.frequency) * freq.weight;
        totalWeight += freq.weight;
    }
    
    return totalWeight > 0 ? value / totalWeight : 0.0;
}

// OTMapGenerator implementation
OTMapGenerator::OTMapGenerator() : noise_generator(nullptr) {
    seedRandom("default");
}

OTMapGenerator::~OTMapGenerator() {
    if (noise_generator) {
        delete noise_generator;
        noise_generator = nullptr;
    }
}

void OTMapGenerator::seedRandom(const std::string& seed) {
    // Convert string seed to numeric seed
    std::hash<std::string> hasher;
    unsigned int numeric_seed = static_cast<unsigned int>(hasher(seed));
    
    // Initialize noise generator and RNG
    delete noise_generator;
    noise_generator = new SimplexNoise(numeric_seed);
    rng.seed(numeric_seed);
}

bool OTMapGenerator::generateMap(BaseMap* map, const GenerationConfig& config) {
    if (!map) {
        return false;
    }
    
    // Cast BaseMap to Map to access the editor's action system
    Map* editorMap = static_cast<Map*>(map);
    
    // Initialize random seed
    seedRandom(config.seed);
    
    // Generate height map
    auto heightMap = generateHeightMap(config);
    
    // Generate terrain layer
    auto terrainLayer = generateTerrainLayer(heightMap, config);
    
    // Apply terrain to map using Actions like the editor does
    std::vector<Position> tilesToGenerate;
    
    // First pass: collect all positions that need tiles
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            uint16_t tileId = terrainLayer[y][x];
            if (tileId != 0) {
                tilesToGenerate.push_back(Position(x, y, config.water_level));
            }
        }
    }
    
    // Process tiles in batches to avoid memory issues
    const int BATCH_SIZE = 1000;
    
    for (size_t start = 0; start < tilesToGenerate.size(); start += BATCH_SIZE) {
        size_t end = std::min(start + BATCH_SIZE, tilesToGenerate.size());
        
        // Create a batch of tiles
        for (size_t i = start; i < end; ++i) {
            Position pos = tilesToGenerate[i];
            uint16_t tileId = terrainLayer[pos.y][pos.x];
            
            // Create tile location and allocate tile properly
            TileLocation* location = editorMap->createTileL(pos);
            Tile* existing_tile = location->get();
            Tile* new_tile = nullptr;
            
            if (existing_tile) {
                // Copy existing tile and modify it
                new_tile = existing_tile->deepCopy(*editorMap);
            } else {
                // Create new tile
                new_tile = editorMap->allocator(location);
            }
            
            // Set the ground using proper API
            if (new_tile) {
                // Remove existing ground if any
                if (new_tile->ground) {
                    delete new_tile->ground;
                    new_tile->ground = nullptr;
                }
                
                // Create new ground item
                Item* groundItem = Item::Create(tileId);
                if (groundItem) {
                    new_tile->ground = groundItem;
                }
                
                // Set the tile back to the map
                editorMap->setTile(pos, new_tile);
            }
        }
    }
    
    // Generate caves if enabled (similar batch approach)
    if (config.add_caves) {
        auto caveLayer = generateCaveLayer(config);
        
        std::vector<Position> caveTilesToGenerate;
        
        for (int y = 0; y < config.height; ++y) {
            for (int x = 0; x < config.width; ++x) {
                uint16_t caveId = caveLayer[y][x];
                if (caveId != 0) {
                    // Place cave tiles below the surface
                    for (int z = config.water_level + 1; z <= config.water_level + config.cave_depth; ++z) {
                        caveTilesToGenerate.push_back(Position(x, y, z));
                    }
                }
            }
        }
        
        // Process cave tiles in batches
        for (size_t start = 0; start < caveTilesToGenerate.size(); start += BATCH_SIZE) {
            size_t end = std::min(start + BATCH_SIZE, caveTilesToGenerate.size());
            
            for (size_t i = start; i < end; ++i) {
                Position pos = caveTilesToGenerate[i];
                uint16_t caveId = caveLayer[pos.y][pos.x];
                
                TileLocation* location = editorMap->createTileL(pos);
                Tile* existing_tile = location->get();
                Tile* new_tile = nullptr;
                
                if (existing_tile) {
                    new_tile = existing_tile->deepCopy(*editorMap);
                } else {
                    new_tile = editorMap->allocator(location);
                }
                
                if (new_tile) {
                    // Remove existing ground if any
                    if (new_tile->ground) {
                        delete new_tile->ground;
                        new_tile->ground = nullptr;
                    }
                    
                    // Create new ground item
                    Item* groundItem = Item::Create(caveId);
                    if (groundItem) {
                        new_tile->ground = groundItem;
                    }
                    
                    // Set the tile back to the map
                    editorMap->setTile(pos, new_tile);
                }
            }
        }
    }
    
    // Add decorations if not terrain only (simplified for now)
    if (!config.terrain_only) {
        for (int y = 0; y < config.height; ++y) {
            for (int x = 0; x < config.width; ++x) {
                Tile* tile = editorMap->getTile(x, y, config.water_level);
                if (tile && tile->ground) {
                    uint16_t groundId = tile->ground->getID();
                    
                    // Add vegetation to grass (simplified)
                    if (groundId == OTMapGenItems::GRASS_TILE_ID) {
                        std::uniform_real_distribution<double> dist(0.0, 1.0);
                        if (dist(rng) < 0.05) { // 5% chance
                            Tile* new_tile = tile->deepCopy(*editorMap);
                            Item* decoration = Item::Create(OTMapGenItems::TREE_ID);
                            if (decoration) {
                                new_tile->addItem(decoration);
                                editorMap->setTile(Position(x, y, config.water_level), new_tile);
                            }
                        }
                    }
                }
            }
        }
    }
    
    return true;
}

std::vector<std::vector<double>> OTMapGenerator::generateHeightMap(const GenerationConfig& config) {
    std::vector<std::vector<double>> heightMap(config.height, std::vector<double>(config.width, 0.0));
    
    double centerX = config.width / 2.0;
    double centerY = config.height / 2.0;
    double maxDistance = std::min(config.width, config.height) / 2.0;
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            // Get base noise value
            double nx = x * config.noise_increment / config.width;
            double ny = y * config.noise_increment / config.height;
            double noiseValue = noise_generator->fractal(nx, ny, config.frequencies);
            
            // Apply island distance effect
            double distance = getDistance(x, y, (int)centerX, (int)centerY, config.euclidean);
            double distanceEffect = 1.0 - pow(distance / maxDistance, config.island_distance_exponent);
            distanceEffect = std::max(0.0, distanceEffect * config.island_distance_decrement);
            
            // Combine noise with distance effect
            double height = (noiseValue + 1.0) * 0.5; // Normalize to [0,1]
            height = pow(height, config.exponent) * config.linear;
            height *= distanceEffect;
            
            heightMap[y][x] = height;
        }
    }
    
    return heightMap;
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateTerrainLayer(const std::vector<std::vector<double>>& heightMap, const GenerationConfig& config) {
    std::vector<std::vector<uint16_t>> terrainLayer(config.height, std::vector<uint16_t>(config.width, 0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double height = heightMap[y][x];
            
            // Generate moisture noise for biome variation
            double moisture = noise_generator->noise(x * 0.01, y * 0.01);
            
            uint16_t tileId = getTerrainTileId(height, moisture, config);
            terrainLayer[y][x] = tileId;
        }
    }
    
    return terrainLayer;
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateCaveLayer(const GenerationConfig& config) {
    std::vector<std::vector<uint16_t>> caveLayer(config.height, std::vector<uint16_t>(config.width, 0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double caveNoise = noise_generator->noise(x * config.cave_roughness, y * config.cave_roughness);
            
            // Random chance for cave generation
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            if (dist(rng) < config.cave_chance && caveNoise > 0.1) {
                caveLayer[y][x] = OTMapGenItems::STONE_TILE_ID; // Cave floor
            }
        }
    }
    
    return caveLayer;
}

uint16_t OTMapGenerator::getTerrainTileId(double height, double moisture, const GenerationConfig& config) {
    // Similar logic to the original getMapTileFromElevation
    if (height < 0.0) {
        return OTMapGenItems::WATER_TILE_ID;
    }
    
    if (height > 0.8) {
        // High elevation - mountains or snow
        if (moisture > 0.2) {
            return OTMapGenItems::STONE_TILE_ID;
        } else {
            return OTMapGenItems::SNOW_TILE_ID;
        }
    }
    
    // Medium to low elevation
    if (config.mountain_type == "SNOW") {
        if (height > 0.1) {
            return OTMapGenItems::SNOW_TILE_ID;
        } else {
            return OTMapGenItems::SNOW_TILE_ID;
        }
    }
    
    // Default terrain generation
    if (config.sand_biome && moisture < -0.6) {
        return OTMapGenItems::SAND_TILE_ID;
    } else {
        return OTMapGenItems::GRASS_TILE_ID;
    }
}

uint16_t OTMapGenerator::getMountainTileId(const std::string& mountainType) {
    if (mountainType == "SNOW") {
        return OTMapGenItems::SNOW_MOUNTAIN_TILE_ID;
    } else if (mountainType == "SAND") {
        return OTMapGenItems::SAND_MOUNTAIN_TILE_ID;
    } else {
        return OTMapGenItems::MOUNTAIN_TILE_ID; // Default mountain type
    }
}

double OTMapGenerator::getDistance(int x, int y, int centerX, int centerY, bool euclidean) {
    if (euclidean) {
        double dx = x - centerX;
        double dy = y - centerY;
        return sqrt(dx * dx + dy * dy);
    } else {
        return std::max(abs(x - centerX), abs(y - centerY));
    }
}

double OTMapGenerator::smoothstep(double edge0, double edge1, double x) {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return x * x * (3 - 2 * x);
}

void OTMapGenerator::generateBorders(BaseMap* map, const GenerationConfig& config) {
    // This would integrate with your existing border system
    // For now, we'll leave this as a placeholder since you already have border generation
}

void OTMapGenerator::addBordersToTile(BaseMap* map, Tile* tile, int x, int y, int z) {
    // Placeholder - integrate with your existing border system
}

void OTMapGenerator::addClutter(BaseMap* map, const GenerationConfig& config) {
    // Add decorative items to terrain
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            Tile* tile = map->getTile(x, y, config.water_level);
            if (tile && tile->ground) {
                uint16_t groundId = tile->ground->getID();
                
                // Add vegetation to grass
                if (groundId == OTMapGenItems::GRASS_TILE_ID) {
                    placeTreesAndVegetation(map, tile, groundId);
                }
                
                // Add stones to various terrains
                if (groundId == OTMapGenItems::GRAVEL_TILE_ID || groundId == OTMapGenItems::STONE_TILE_ID) {
                    placeStones(map, tile, groundId);
                }
            }
        }
    }
}

void OTMapGenerator::placeTreesAndVegetation(BaseMap* map, Tile* tile, uint16_t groundId) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    // Random chance for vegetation
    if (dist(rng) < 0.1) { // 10% chance
        uint16_t decorationId;
        double rand_val = dist(rng);
        
        if (rand_val < 0.6) {
            decorationId = OTMapGenItems::TREE_ID;
        } else if (rand_val < 0.8) {
            decorationId = OTMapGenItems::BUSH_ID;
        } else {
            decorationId = OTMapGenItems::FLOWER_ID;
        }
        
        OTMapGenUtils::addDecoration(tile, decorationId);
    }
}

void OTMapGenerator::placeStones(BaseMap* map, Tile* tile, uint16_t groundId) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    // Random chance for stones
    if (dist(rng) < 0.05) { // 5% chance
        uint16_t stoneId = (dist(rng) < 0.7) ? OTMapGenItems::SMALL_STONE_ID : OTMapGenItems::LARGE_STONE_ID;
        OTMapGenUtils::addDecoration(tile, stoneId);
    }
}

void OTMapGenerator::placeCaveDecorations(BaseMap* map, Tile* tile) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    // Random chance for cave decorations
    if (dist(rng) < 0.15) { // 15% chance
        OTMapGenUtils::addDecoration(tile, OTMapGenItems::STALAGMITE_ID);
    }
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateLayers(const GenerationConfig& config) {
    // Initialize random seed
    seedRandom(config.seed);
    
    // Generate height map
    auto heightMap = generateHeightMap(config);
    
    // Create 8 layers (floors) like the original OTMapGen
    std::vector<std::vector<std::vector<uint16_t>>> layers(8);
    for (int z = 0; z < 8; ++z) {
        layers[z] = std::vector<std::vector<uint16_t>>(config.height, std::vector<uint16_t>(config.width, 0));
    }
    
    // Fill terrain using the fillColumn approach like the original
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double height = heightMap[y][x];
            
            // Generate moisture noise for biome variation
            double moisture = noise_generator->noise(x * 0.01, y * 0.01);
            
            // Get terrain tile ID
            uint16_t tileId = getTerrainTileId(height, moisture, config);
            
            // Calculate elevation for determining elevated content
            // Scale height to 0-7 range for determining what gets elevated content
            int elevation = static_cast<int>(std::max(0.0, std::min(7.0, height * 8.0)));
            
            // Fill column - this will put main terrain on layers[0] (Floor 7)
            // and add elevated content based on elevation value
            fillColumn(layers, x, y, elevation, tileId, config);
        }
    }
    
    // Convert 3D layers to the format expected by the dialog (single layer for each floor)
    // Return layers in the order they map to Tibia floors:
    // layers[0] → Floor 7 (ground level)
    // layers[1] → Floor 6 (+1 above)
    // layers[7] → Floor 0 (+7 above)
    std::vector<std::vector<uint16_t>> result;
    for (int z = 0; z < 8; ++z) {
        std::vector<uint16_t> floorData;
        floorData.reserve(config.width * config.height);
        
        for (int y = 0; y < config.height; ++y) {
            for (int x = 0; x < config.width; ++x) {
                floorData.push_back(layers[z][y][x]);
            }
        }
        result.push_back(floorData);
    }
    
    return result;
}

void OTMapGenerator::fillColumn(std::vector<std::vector<std::vector<uint16_t>>>& layers, 
                               int x, int y, int elevation, uint16_t surfaceTileId, 
                               const GenerationConfig& config) {
    // Always place the surface tile on the ground level (layers[0] → Floor 7)
    layers[0][y][x] = surfaceTileId;
    
    // For elevated terrain, we can optionally fill some above-ground floors
    // with the same terrain or sparse variations
    if (elevation > 4) { // High elevation areas
        // Add some terrain to the first above-ground floor
        layers[1][y][x] = surfaceTileId;
        
        if (elevation > 6) { // Very high elevation
            // Add sparse terrain to second above-ground floor
            layers[2][y][x] = surfaceTileId;
        }
    }
    
    // Note: We're not filling "below" anymore since we're not doing underground yet
    // All the main terrain goes on the ground level (layers[0] → Floor 7)
    // Above-ground floors (layers[1-7] → Floors 6-0) get sparse elevated content
}

// Utility functions (no longer needed - using Action system instead)



namespace OTMapGenUtils {
    Tile* getOrCreateTile(BaseMap* map, int x, int y, int z) {
        Position pos(x, y, z);
        Tile* tile = map->getTile(pos);
        if (!tile) {
            tile = map->allocator(map->createTileL(pos));
            map->setTile(pos, tile);
        }
        return tile;
    }
    
    void setGroundTile(Tile* tile, uint16_t itemId) {
        if (!tile) return;
        
        // Remove existing ground
        if (tile->ground) {
            delete tile->ground;
            tile->ground = nullptr;
        }
        
        // Create new ground item
        Item* groundItem = Item::Create(itemId);
        if (groundItem) {
            tile->ground = groundItem;
        }
    }
    
    void addDecoration(Tile* tile, uint16_t itemId) {
        if (!tile) return;
        
        Item* decoration = Item::Create(itemId);
        if (decoration) {
            tile->addItem(decoration);
        }
    }
    
    bool isWaterTile(uint16_t itemId) {
        return itemId == OTMapGenItems::WATER_TILE_ID;
    }
    
    bool isLandTile(uint16_t itemId) {
        return itemId == OTMapGenItems::GRASS_TILE_ID || 
               itemId == OTMapGenItems::SAND_TILE_ID ||
               itemId == OTMapGenItems::STONE_TILE_ID ||
               itemId == OTMapGenItems::GRAVEL_TILE_ID;
    }
    
    bool isMountainTile(uint16_t itemId) {
        return itemId == OTMapGenItems::MOUNTAIN_TILE_ID ||
               itemId == OTMapGenItems::SNOW_MOUNTAIN_TILE_ID ||
               itemId == OTMapGenItems::SAND_MOUNTAIN_TILE_ID;
    }
} 
