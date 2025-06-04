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
    // Try to parse as 64-bit integer first, like original OTMapGen
    unsigned long long numeric_seed = 0;
    
    try {
        // Try to parse as number
        numeric_seed = std::stoull(seed);
    } catch (...) {
        // If parsing fails, fall back to string hash
        std::hash<std::string> hasher;
        numeric_seed = hasher(seed);
    }
    
    // Initialize noise generator and RNG with 64-bit seed
    delete noise_generator;
    noise_generator = new SimplexNoise(static_cast<unsigned int>(numeric_seed & 0xFFFFFFFF));
    rng.seed(static_cast<unsigned int>(numeric_seed >> 32) ^ static_cast<unsigned int>(numeric_seed & 0xFFFFFFFF));
}

bool OTMapGenerator::generateMap(BaseMap* map, const GenerationConfig& config) {
    if (!map) {
        return false;
    }
    
    // Cast BaseMap to Map to access the editor's action system
    Map* editorMap = static_cast<Map*>(map);
    
    // Initialize random seed
    seedRandom(config.seed);
    
    // Generate height map and moisture map
    auto heightMap = generateHeightMap(config);
    auto moistureMap = generateMoistureMap(config);
    
    // Generate terrain layer
    auto terrainLayer = generateTerrainLayer(heightMap, moistureMap, config);
    
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
    
    // Generate caves if enabled
    if (config.add_caves) {
        auto caveLayer = generateCaveLayer(config);
        
        std::vector<Position> caveTilesToGenerate;
        
        for (int y = 0; y < config.height; ++y) {
            for (int x = 0; x < config.width; ++x) {
                uint16_t caveId = caveLayer[y][x];
                if (caveId != 0) {
                    // Place cave tiles below the surface (floors 8+)
                    // Since map editor uses positive Z for underground, use water_level + offset
                    for (int z = config.water_level + 1; z <= config.water_level + config.cave_depth && z <= 15; ++z) {
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
        // Find grass layer for decoration placement
        const TerrainLayer* grassLayer = nullptr;
        for (const auto& layer : config.terrain_layers) {
            if (layer.name == "Grass" && layer.enabled) {
                grassLayer = &layer;
                break;
            }
        }
        
        if (grassLayer) {
            for (int y = 0; y < config.height; ++y) {
                for (int x = 0; x < config.width; ++x) {
                    Tile* tile = editorMap->getTile(x, y, config.water_level);
                    if (tile && tile->ground) {
                        uint16_t groundId = tile->ground->getID();
                        
                        // Add vegetation to grass tiles
                        if (groundId == grassLayer->item_id) {
                            std::uniform_real_distribution<double> dist(0.0, 1.0);
                            if (dist(rng) < 0.05) { // 5% chance
                                Tile* new_tile = tile->deepCopy(*editorMap);
                                
                                // Use configurable decoration items (could be made configurable too)
                                uint16_t decorationId = 2700; // Default tree ID
                                double rand_val = dist(rng);
                                if (rand_val < 0.6) {
                                    decorationId = 2700; // Tree
                                } else if (rand_val < 0.8) {
                                    decorationId = 2785; // Bush
                                } else {
                                    decorationId = 2782; // Flower
                                }
                                
                                Item* decoration = Item::Create(decorationId);
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
            // Enhanced noise for better island shapes
            double nx = x * config.noise_increment / config.width;
            double ny = y * config.noise_increment / config.height;
            
            // Multi-octave noise for more interesting terrain
            double noiseValue = 0.0;
            double amplitude = 1.0;
            double frequency = 1.0;
            double maxValue = 0.0;
            
            // Generate 4 octaves of noise for complex island shapes
            for (int i = 0; i < 4; i++) {
                noiseValue += noise_generator->noise(nx * frequency, ny * frequency) * amplitude;
                maxValue += amplitude;
                amplitude *= 0.5;
                frequency *= 2.0;
            }
            noiseValue /= maxValue; // Normalize
            
            // Apply island distance effect with better shaping
            double distance = getDistance(x, y, (int)centerX, (int)centerY, config.euclidean);
            double normalizedDistance = distance / maxDistance;
            
            // Create more interesting island shapes with noise-based distortion
            double distortionNoise = noise_generator->noise(x * 0.01, y * 0.01) * 0.3;
            normalizedDistance += distortionNoise;
            
            // Improved island falloff for organic shapes
            double distanceEffect = 1.0 - pow(std::max(0.0, normalizedDistance), config.island_distance_exponent);
            distanceEffect = std::max(0.0, distanceEffect * config.island_distance_decrement);
            
            // Apply sharper transitions for distinct land/water boundaries
            if (distanceEffect < 0.3) {
                distanceEffect *= distanceEffect; // Square it to create sharper dropoff
            }
            
            // Combine noise with distance effect
            double height = (noiseValue + 1.0) * 0.5; // Normalize to [0,1]
            height = pow(height, config.exponent) * config.linear;
            height *= distanceEffect;
            
            // Add some randomness to break up regular patterns
            height += (noise_generator->noise(x * 0.1, y * 0.1) * 0.05);
            height = std::max(0.0, std::min(1.0, height));
            
            heightMap[y][x] = height;
        }
    }
    
    return heightMap;
}

std::vector<std::vector<double>> OTMapGenerator::generateMoistureMap(const GenerationConfig& config) {
    std::vector<std::vector<double>> moistureMap(config.height, std::vector<double>(config.width, 0.0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            // Generate moisture noise with different scale than height
            double nx = x * 0.01; // Moisture varies more slowly
            double ny = y * 0.01;
            double moistureValue = noise_generator->noise(nx, ny);
            
            moistureMap[y][x] = moistureValue;
        }
    }
    
    return moistureMap;
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateTerrainLayer(const std::vector<std::vector<double>>& heightMap, 
                                                      const std::vector<std::vector<double>>& moistureMap,
                                                      const GenerationConfig& config) {
    std::vector<std::vector<uint16_t>> terrainLayer(config.height, std::vector<uint16_t>(config.width, 0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double height = heightMap[y][x];
            double moisture = moistureMap[y][x];
            
            // Reduce moisture influence for more consistent terrain generation
            // This helps prevent strange terrain combinations on upper floors
            uint16_t tileId = getTerrainTileId(height, moisture * 0.7, config);
            terrainLayer[y][x] = tileId;
        }
    }
    
    return terrainLayer;
}

uint16_t OTMapGenerator::getTerrainTileId(double height, double moisture, const GenerationConfig& config) {
    // Select terrain layer based on height and reduced moisture influence
    const TerrainLayer* selectedLayer = selectTerrainLayer(height, moisture, config);
    
    if (selectedLayer) {
        return selectedLayer->item_id;
    }
    
    // Fallback to water if no layer matches
    return config.water_item_id;
}

const TerrainLayer* OTMapGenerator::selectTerrainLayer(double height, double moisture, const GenerationConfig& config) {
    // Sort layers by z-order (higher z-order = higher priority)
    std::vector<const TerrainLayer*> sortedLayers;
    for (const auto& layer : config.terrain_layers) {
        if (layer.enabled) {
            sortedLayers.push_back(&layer);
        }
    }
    
    std::sort(sortedLayers.begin(), sortedLayers.end(), 
        [](const TerrainLayer* a, const TerrainLayer* b) {
            return a->z_order > b->z_order; // Higher z-order first
        });
    
    // Find the first layer that matches the height and moisture criteria
    for (const TerrainLayer* layer : sortedLayers) {
        if (height >= layer->height_min && height <= layer->height_max &&
            moisture >= layer->moisture_min && moisture <= layer->moisture_max) {
            
            // Check coverage probability
            if (layer->coverage >= 1.0) {
                return layer;
            } else {
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                if (dist(rng) < layer->coverage) {
                    return layer;
                }
            }
        }
    }
    
    return nullptr; // No matching layer found
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateCaveLayer(const GenerationConfig& config) {
    std::vector<std::vector<uint16_t>> caveLayer(config.height, std::vector<uint16_t>(config.width, 0));
    
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double caveNoise = noise_generator->noise(x * config.cave_roughness, y * config.cave_roughness);
            
            // Random chance for cave generation
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            if (dist(rng) < config.cave_chance && caveNoise > 0.1) {
                caveLayer[y][x] = config.cave_item_id; // Use configurable cave item ID
            }
        }
    }
    
    return caveLayer;
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
                
                // Find the terrain layer this ground belongs to
                const TerrainLayer* terrainLayer = nullptr;
                for (const auto& layer : config.terrain_layers) {
                    if (layer.item_id == groundId && layer.enabled) {
                        terrainLayer = &layer;
                        break;
                    }
                }
                
                if (terrainLayer) {
                    // Add decorations based on terrain type
                    if (terrainLayer->name == "Grass") {
                        placeTreesAndVegetation(map, tile, groundId);
                    } else if (terrainLayer->name == "Mountain" || terrainLayer->brush_name.find("stone") != std::string::npos) {
                        placeStones(map, tile, groundId);
                    }
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
            decorationId = 2700; // Tree
        } else if (rand_val < 0.8) {
            decorationId = 2785; // Bush
        } else {
            decorationId = 2782; // Flower
        }
        
        OTMapGenUtils::addDecoration(tile, decorationId);
    }
}

void OTMapGenerator::placeStones(BaseMap* map, Tile* tile, uint16_t groundId) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    // Random chance for stones
    if (dist(rng) < 0.05) { // 5% chance
        uint16_t stoneId = (dist(rng) < 0.7) ? 1770 : 1771; // Small or large stone
        OTMapGenUtils::addDecoration(tile, stoneId);
    }
}

void OTMapGenerator::placeCaveDecorations(BaseMap* map, Tile* tile) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    // Random chance for cave decorations
    if (dist(rng) < 0.15) { // 15% chance
        OTMapGenUtils::addDecoration(tile, 1785); // Stalagmite
    }
}

std::vector<std::vector<uint16_t>> OTMapGenerator::generateLayers(const GenerationConfig& config) {
    // Initialize random seed
    seedRandom(config.seed);
    
    // Generate height map and moisture map
    auto heightMap = generateHeightMap(config);
    auto moistureMap = generateMoistureMap(config);
    
    // Create 8 layers (floors) like the original OTMapGen
    std::vector<std::vector<std::vector<uint16_t>>> layers(8);
    for (int z = 0; z < 8; ++z) {
        layers[z] = std::vector<std::vector<uint16_t>>(config.height, std::vector<uint16_t>(config.width, 0));
    }
    
    // Fill terrain using the fillColumn approach like the original
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            double height = heightMap[y][x];
            double moisture = moistureMap[y][x];
            
            // Get terrain tile ID using the new configurable system
            uint16_t tileId = getTerrainTileId(height, moisture, config);
            
            // Calculate elevation for determining elevated content
            // Scale height to 0-7 range for determining what gets elevated content
            int elevation = static_cast<int>(std::max(0.0, std::min(7.0, height * 8.0)));
            
            // Fill column - this will put main terrain on layers[0] (Floor 7)
            // and add elevated content based on elevation value
            fillColumn(layers, x, y, elevation, tileId, config);
        }
    }
    
    // Add caves if enabled (now using configurable cave generation)
    if (config.add_caves) {
        auto caveLayer = generateCaveLayer(config);
        
        // Place caves on underground floors (below the main surface)
        // In Tibia coordinates: floors 8+ are underground
        // But since we only have 8 layers (0-7), we'll place caves on upper floors sparsely
        for (int y = 0; y < config.height; ++y) {
            for (int x = 0; x < config.width; ++x) {
                uint16_t caveId = caveLayer[y][x];
                if (caveId != 0) {
                    // Place caves on layers 1-3 (floors 6-4) with decreasing probability
                    std::uniform_real_distribution<double> dist(0.0, 1.0);
                    
                    if (dist(rng) < 0.8) { // 80% chance on floor 6
                        layers[1][y][x] = caveId;
                    }
                    if (dist(rng) < 0.5) { // 50% chance on floor 5  
                        layers[2][y][x] = caveId;
                    }
                    if (dist(rng) < 0.2) { // 20% chance on floor 4
                        layers[3][y][x] = caveId;
                    }
                }
            }
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
    
    // For upper floors, we need different logic than just elevation
    // Upper floors should create proper mountain formations with grass/ground between rock layers
    
    if (elevation > 4) { // Start considering upper floors at medium elevation
        // Check if this is mountain terrain by item ID or brush name
        bool isMountainTerrain = false;
        for (const auto& layer : config.terrain_layers) {
            if (layer.item_id == surfaceTileId && layer.enabled) {
                isMountainTerrain = (layer.name == "Mountain" || layer.brush_name == "mountain" || 
                                   layer.brush_name == "snow" || layer.brush_name.find("stone") != std::string::npos);
                break;
            }
        }
        
        if (isMountainTerrain) {
            // Generate mountain formations with proper layering
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            
            // Generate additional noise for upper floor variation
            double upperNoise = noise_generator->noise(x * 0.05, y * 0.05);
            double verticalNoise = noise_generator->noise(x * 0.02, y * 0.02);
            
            // Calculate probability based on elevation and noise
            double mountainChance = (elevation - 4) / 4.0; // 0-1 based on elevation above threshold
            mountainChance *= (upperNoise + 1.0) * 0.5; // Modulate with noise
            
            // Layer 1 (Floor 6): More frequent mountain/rock
            if (dist(rng) < mountainChance * 0.7) {
                layers[1][y][x] = surfaceTileId;
                
                // Layer 2 (Floor 5): Sometimes grass/ground between rock layers
                if (elevation > 6 && dist(rng) < 0.4) {
                    if (verticalNoise > 0.2) {
                        layers[2][y][x] = surfaceTileId; // Continue mountain
                    } else {
                        // Create ground/grass layer between rock formations
                        const TerrainLayer* grassLayer = nullptr;
                        for (const auto& layer : config.terrain_layers) {
                            if ((layer.name == "Grass" || layer.brush_name == "grass") && layer.enabled) {
                                grassLayer = &layer;
                                break;
                            }
                        }
                        if (grassLayer) {
                            layers[2][y][x] = grassLayer->item_id;
                        }
                    }
                    
                    // Layer 3 (Floor 4): Final rock cap if very high elevation
                    if (elevation > 7 && verticalNoise > 0.5 && dist(rng) < 0.3) {
                        layers[3][y][x] = surfaceTileId;
                    }
                }
            }
        }
    }
    
    // Note: Underground content (caves) is handled separately in generateLayers
}

// Utility functions
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
        // This could be made configurable by checking against water item IDs
        return itemId == 4608 || itemId == 4609 || itemId == 4610 || itemId == 4611;
    }
    
    bool isLandTile(uint16_t itemId) {
        // This could be made configurable by checking against land item IDs  
        return itemId == 4526 || itemId == 231 || itemId == 1284 || itemId == 4597;
    }
    
    bool isMountainTile(uint16_t itemId) {
        // This could be made configurable by checking against mountain item IDs
        return itemId == 919 || itemId == 4611 || itemId == 4621 || itemId == 4616;
    }
    
    std::vector<std::string> getAvailableBrushes() {
        // This should parse the actual grounds.xml files
        // For now, return a basic list
        return {
            "grass", "sea", "sand", "mountain", "cave", "snow", 
            "stone floor", "wooden floor", "lawn", "ice"
        };
    }
    
    uint16_t getPrimaryItemFromBrush(const std::string& brushName) {
        // This should parse the grounds.xml to get the primary item ID
        // For now, use basic mappings
        if (brushName == "grass") return 4526;
        else if (brushName == "sea") return 4608;
        else if (brushName == "sand") return 231;
        else if (brushName == "mountain") return 919;
        else if (brushName == "cave") return 351;
        else if (brushName == "snow") return 670;
        else if (brushName == "stone floor") return 431;
        else if (brushName == "wooden floor") return 405;
        else if (brushName == "lawn") return 106;
        else if (brushName == "ice") return 671;
        return 100; // Default
    }
    
    bool applyBrushToTile(BaseMap* map, Tile* tile, const std::string& brushName, int x, int y, int z) {
        // This should integrate with the actual brush system
        // For now, just set the ground tile
        uint16_t itemId = getPrimaryItemFromBrush(brushName);
        setGroundTile(tile, itemId);
        return true;
    }
} 
