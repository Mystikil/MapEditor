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

#ifndef RME_OTMAPGEN_BASE_H_
#define RME_OTMAPGEN_BASE_H_

#include <string>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <functional>
#include <iostream>
#include <queue>
#include <map>

class BaseMap;

// Base configuration structures
struct FrequencyWeight {
    double frequency;
    double weight;
    
    FrequencyWeight(double f, double w) : frequency(f), weight(w) {}
};

struct TerrainLayer {
    std::string name;
    std::string brush_name;
    uint16_t item_id;
    double height_min;
    double height_max;
    double moisture_min;
    double moisture_max;
    double coverage;
    int z_order;
    bool enabled;
    
    TerrainLayer() : 
        item_id(0), height_min(0.0), height_max(1.0), 
        moisture_min(-1.0), moisture_max(1.0), coverage(1.0), 
        z_order(0), enabled(true) {}
};

struct GenerationConfig {
    std::string seed;
    int width;
    int height;
    int base_floor;
    int water_level;
    
    // Noise settings
    double noise_increment;
    double exponent;
    double linear;
    double island_distance_exponent;
    double island_distance_decrement;
    bool euclidean;
    
    // Terrain settings
    std::vector<TerrainLayer> terrain_layers;
    uint16_t water_item_id;
    bool terrain_only;
    
    // Cave settings
    bool add_caves;
    int cave_depth;
    double cave_roughness;
    double cave_chance;
    uint16_t cave_item_id;
    
    // Multi-floor settings
    int floors_to_generate;
    
    std::vector<FrequencyWeight> frequencies;
    
    GenerationConfig() :
        seed("default"), width(100), height(100), base_floor(7), water_level(7),
        noise_increment(0.1), exponent(1.0), linear(1.0), 
        island_distance_exponent(2.0), island_distance_decrement(1.0), euclidean(true),
        water_item_id(4608), terrain_only(false),
        add_caves(false), cave_depth(3), cave_roughness(0.05), cave_chance(0.1), cave_item_id(351),
        floors_to_generate(1) {}
};

// Simple Noise Generator
class SimplexNoise {
public:
    SimplexNoise(unsigned int seed);
    double noise(double x, double y);
    double fractal(double x, double y, const std::vector<FrequencyWeight>& frequencies);
    
private:
    void initializePermutation(unsigned int seed);
    int fastfloor(double x);
    double dot(const int g[], double x, double y);
    
    static const double F2;
    static const double G2;
    static const int SIMPLEX[64][4];
    
    int perm[512];
    int permMod12[512];
};

// Base Map Generator class
class OTMapGeneratorBase {
public:
    OTMapGeneratorBase();
    virtual ~OTMapGeneratorBase();
    
    // Core functionality
    void seedRandom(const std::string& seed);
    
    // Utility functions
    double getDistance(int x, int y, int centerX, int centerY, bool euclidean);
    double smoothstep(double edge0, double edge1, double x);
    
    // Basic terrain generation
    std::vector<std::vector<double>> generateHeightMap(const GenerationConfig& config);
    std::vector<std::vector<double>> generateMoistureMap(const GenerationConfig& config);
    
protected:
    SimplexNoise* noise_generator;
    std::mt19937 rng;
    
    // Helper methods for derived classes
    const TerrainLayer* selectTerrainLayer(double height, double moisture, const GenerationConfig& config);
    uint16_t getTerrainTileId(double height, double moisture, const GenerationConfig& config);
};

// Utility namespace
namespace OTMapGenUtils {
    class Tile;
    class BaseMap;
    
    Tile* getOrCreateTile(BaseMap* map, int x, int y, int z);
    void setGroundTile(Tile* tile, uint16_t itemId);
    void addDecoration(Tile* tile, uint16_t itemId);
    bool isWaterTile(uint16_t itemId);
    bool isLandTile(uint16_t itemId);
    bool isMountainTile(uint16_t itemId);
    std::vector<std::string> getAvailableBrushes();
    uint16_t getPrimaryItemFromBrush(const std::string& brushName);
    bool applyBrushToTile(BaseMap* map, Tile* tile, const std::string& brushName, int x, int y, int z);
}

#endif 