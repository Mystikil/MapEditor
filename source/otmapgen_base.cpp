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
#include "otmapgen_base.h"
#include "position.h"
#include "basemap.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include "map.h"

// Type aliases for the utility namespace
namespace OTMapGenUtils {
    using BaseMap = Map;
    using Tile = ::Tile;
}

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

double SimplexNoise::fractal(double x, double y, const std::vector<FrequencyWeight>& frequencies) {
    double value = 0.0;
    double totalWeight = 0.0;
    
    for (const auto& freq : frequencies) {
        value += noise(x * freq.frequency, y * freq.frequency) * freq.weight;
        totalWeight += freq.weight;
    }
    
    return totalWeight > 0 ? value / totalWeight : 0.0;
}

// OTMapGeneratorBase implementation
OTMapGeneratorBase::OTMapGeneratorBase() : noise_generator(nullptr) {
    seedRandom("default");
}

OTMapGeneratorBase::~OTMapGeneratorBase() {
    if (noise_generator) {
        delete noise_generator;
        noise_generator = nullptr;
    }
}

void OTMapGeneratorBase::seedRandom(const std::string& seed) {
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

double OTMapGeneratorBase::getDistance(int x, int y, int centerX, int centerY, bool euclidean) {
    if (euclidean) {
        double dx = x - centerX;
        double dy = y - centerY;
        return sqrt(dx * dx + dy * dy);
    } else {
        // Manhattan distance
        return abs(x - centerX) + abs(y - centerY);
    }
}

double OTMapGeneratorBase::smoothstep(double edge0, double edge1, double x) {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return x * x * (3 - 2 * x);
}

std::vector<std::vector<double>> OTMapGeneratorBase::generateHeightMap(const GenerationConfig& config) {
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

std::vector<std::vector<double>> OTMapGeneratorBase::generateMoistureMap(const GenerationConfig& config) {
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

const TerrainLayer* OTMapGeneratorBase::selectTerrainLayer(double height, double moisture, const GenerationConfig& config) {
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

uint16_t OTMapGeneratorBase::getTerrainTileId(double height, double moisture, const GenerationConfig& config) {
    const TerrainLayer* layer = selectTerrainLayer(height, moisture, config);
    return layer ? layer->item_id : 100; // Default fallback
}

// GenerationConfig implementation
void GenerationConfig::initializeDefaultLayers() {
    terrain_layers.clear();
    
    // Default Water layer
    TerrainLayer waterLayer;
    waterLayer.name = "Water";
    waterLayer.brush_name = "sea";
    waterLayer.item_id = 4608;
    waterLayer.height_min = -1.0;
    waterLayer.height_max = 0.2;
    waterLayer.moisture_min = -1.0;
    waterLayer.moisture_max = 1.0;
    waterLayer.noise_scale = 1.0;
    waterLayer.coverage = 1.0;
    waterLayer.z_order = 100;
    waterLayer.use_borders = true;
    waterLayer.enabled = true;
    terrain_layers.push_back(waterLayer);
    
    // Default Sand layer
    TerrainLayer sandLayer;
    sandLayer.name = "Sand";
    sandLayer.brush_name = "sand";
    sandLayer.item_id = 231;
    sandLayer.height_min = 0.1;
    sandLayer.height_max = 0.3;
    sandLayer.moisture_min = -1.0;
    sandLayer.moisture_max = 0.5;
    sandLayer.noise_scale = 1.0;
    sandLayer.coverage = 1.0;
    sandLayer.z_order = 200;
    sandLayer.use_borders = true;
    sandLayer.enabled = true;
    terrain_layers.push_back(sandLayer);
    
    // Default Grass layer
    TerrainLayer grassLayer;
    grassLayer.name = "Grass";
    grassLayer.brush_name = "grass";
    grassLayer.item_id = 4526;
    grassLayer.height_min = 0.2;
    grassLayer.height_max = 0.7;
    grassLayer.moisture_min = 0.0;
    grassLayer.moisture_max = 1.0;
    grassLayer.noise_scale = 1.0;
    grassLayer.coverage = 1.0;
    grassLayer.z_order = 300;
    grassLayer.use_borders = true;
    grassLayer.enabled = true;
    terrain_layers.push_back(grassLayer);
    
    // Default Mountain layer
    TerrainLayer mountainLayer;
    mountainLayer.name = "Mountain";
    mountainLayer.brush_name = "mountain";
    mountainLayer.item_id = 919;
    mountainLayer.height_min = 0.6;
    mountainLayer.height_max = 1.0;
    mountainLayer.moisture_min = -1.0;
    mountainLayer.moisture_max = 1.0;
    mountainLayer.noise_scale = 1.0;
    mountainLayer.coverage = 1.0;
    mountainLayer.z_order = 400;
    mountainLayer.use_borders = true;
    mountainLayer.enabled = true;
    terrain_layers.push_back(mountainLayer);
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