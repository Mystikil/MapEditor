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

#ifndef RME_OTMAPGEN_H_
#define RME_OTMAPGEN_H_

#include "main.h"
#include <vector>
#include <string>
#include <map>
#include <random>

// Forward declarations
class BaseMap;
class Tile;

struct GenerationConfig {
    std::string seed;
    int width = 256;
    int height = 256;
    std::string version = "10.98";
    bool terrain_only = false;
    
    // Generation parameters
    double noise_increment = 1.0;
    double island_distance_decrement = 0.92;
    double island_distance_exponent = 0.25;
    int cave_depth = 20;
    double cave_roughness = 0.45;
    double cave_chance = 0.09;
    bool sand_biome = true;
    bool euclidean = false;
    bool smooth_coastline = true;
    bool add_caves = true;
    int water_level = 2;
    double exponent = 1.4;
    double linear = 6.0;
    std::string mountain_type = "MOUNTAIN";
    
    // Frequency weights for noise generation
    struct FrequencyWeight {
        double frequency;
        double weight;
    };
    
    std::vector<FrequencyWeight> frequencies = {
        {1.0, 0.3}, {2.0, 0.2}, {4.0, 0.2}, {8.0, 0.125},
        {16.0, 0.1}, {32.0, 0.05}, {64.0, 0.0025}
    };
};

class SimplexNoise {
public:
    SimplexNoise(unsigned int seed = 0);
    
    double noise(double x, double y);
    double fractal(double x, double y, const std::vector<GenerationConfig::FrequencyWeight>& frequencies);
    
private:
    static const int SIMPLEX[64][4];
    static const double F2, G2;
    
    int perm[512];
    int permMod12[512];
    
    void initializePermutation(unsigned int seed);
    double dot(const int g[], double x, double y);
    int fastfloor(double x);
};

class OTMapGenerator {
public:
    OTMapGenerator();
    ~OTMapGenerator();
    
    // Main generation function
    bool generateMap(BaseMap* map, const GenerationConfig& config);
    
    // Preview generation
    std::vector<std::vector<uint16_t>> generateLayers(const GenerationConfig& config);
    
    // Core generation methods
    std::vector<std::vector<double>> generateHeightMap(const GenerationConfig& config);
    std::vector<std::vector<uint16_t>> generateTerrainLayer(const std::vector<std::vector<double>>& heightMap, const GenerationConfig& config);
    
    // Helper methods for multi-floor generation
    void fillColumn(std::vector<std::vector<std::vector<uint16_t>>>& layers, 
                   int x, int y, int elevation, uint16_t surfaceTileId, 
                   const GenerationConfig& config);
    uint16_t getTerrainTileId(double height, double moisture, const GenerationConfig& config);
    uint16_t getMountainTileId(const std::string& mountainType);
    
private:
    SimplexNoise* noise_generator;
    std::mt19937 rng;
    
    // Cave generation
    std::vector<std::vector<uint16_t>> generateCaveLayer(const GenerationConfig& config);
    
    // Utility functions
    double getDistance(int x, int y, int centerX, int centerY, bool euclidean = false);
    double smoothstep(double edge0, double edge1, double x);
    void seedRandom(const std::string& seed);
    
    // Border generation
    void generateBorders(BaseMap* map, const GenerationConfig& config);
    void addBordersToTile(BaseMap* map, Tile* tile, int x, int y, int z);
    
    // Decoration placement
    void addClutter(BaseMap* map, const GenerationConfig& config);
    void placeTreesAndVegetation(BaseMap* map, Tile* tile, uint16_t groundId);
    void placeStones(BaseMap* map, Tile* tile, uint16_t groundId);
    void placeCaveDecorations(BaseMap* map, Tile* tile);
};

// Item ID constants (matching the JavaScript version)
namespace OTMapGenItems {
    const uint16_t WATER_TILE_ID = 4608;       // Water
    const uint16_t GRASS_TILE_ID = 4526;       // Grass
    const uint16_t SAND_TILE_ID = 231;         // Sand
    const uint16_t STONE_TILE_ID = 1284;       // Stone
    const uint16_t GRAVEL_TILE_ID = 4597;      // Gravel
    const uint16_t SNOW_TILE_ID = 670;         // Snow
    
    // Mountain types
    const uint16_t MOUNTAIN_TILE_ID = 4611;    // Mountain
    const uint16_t SNOW_MOUNTAIN_TILE_ID = 4621; // Snow mountain
    const uint16_t SAND_MOUNTAIN_TILE_ID = 4616; // Sand mountain
    
    // Vegetation
    const uint16_t TREE_ID = 2700;             // Tree
    const uint16_t BUSH_ID = 2785;             // Bush
    const uint16_t FLOWER_ID = 2782;           // Flowers
    
    // Stones and decorations
    const uint16_t SMALL_STONE_ID = 1770;      // Small stone
    const uint16_t LARGE_STONE_ID = 1771;      // Large stone
    
    // Cave items
    const uint16_t CAVE_WALL_ID = 1284;        // Cave wall
    const uint16_t STALAGMITE_ID = 1785;       // Stalagmite
}

// Helper functions for tile creation and manipulation
namespace OTMapGenUtils {
    Tile* getOrCreateTile(BaseMap* map, int x, int y, int z);
    void setGroundTile(Tile* tile, uint16_t itemId);
    void addDecoration(Tile* tile, uint16_t itemId);
    bool isWaterTile(uint16_t itemId);
    bool isLandTile(uint16_t itemId);
    bool isMountainTile(uint16_t itemId);
}

#endif 