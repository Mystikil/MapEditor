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

// Terrain layer configuration for multiple ground types
struct TerrainLayer {
    std::string name;
    std::string brush_name;        // Name of brush from grounds.xml
    uint16_t item_id;             // Primary item ID for this layer
    double height_min = 0.0;      // Minimum height for this terrain
    double height_max = 1.0;      // Maximum height for this terrain
    double moisture_min = -1.0;   // Minimum moisture for this terrain
    double moisture_max = 1.0;    // Maximum moisture for this terrain
    double noise_scale = 1.0;     // Scale factor for noise sampling
    double coverage = 1.0;        // Coverage probability (0.0 to 1.0)
    bool use_borders = true;      // Whether to apply borders from brush
    bool enabled = true;          // Whether this layer is active
    int z_order = 1000;          // Rendering order (higher = on top)
};

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
    int water_level = 7;  // Changed to match Tibia coordinates (7 = ground level)
    double exponent = 1.4;
    double linear = 6.0;
    std::string mountain_type = "MOUNTAIN";
    
    // Configurable terrain layers instead of hardcoded items
    std::vector<TerrainLayer> terrain_layers;
    
    // Cave configuration
    std::string cave_brush_name = "cave";
    uint16_t cave_item_id = 351;  // Default cave floor from grounds.xml
    
    // Water configuration  
    std::string water_brush_name = "sea";
    uint16_t water_item_id = 4608;  // Default water from grounds.xml
    
    // Frequency weights for noise generation
    struct FrequencyWeight {
        double frequency;
        double weight;
    };
    
    std::vector<FrequencyWeight> frequencies = {
        {1.0, 0.3}, {2.0, 0.2}, {4.0, 0.2}, {8.0, 0.125},
        {16.0, 0.1}, {32.0, 0.05}, {64.0, 0.0025}
    };
    
    // Initialize with default terrain layers
    void initializeDefaultLayers() {
        terrain_layers.clear();
        
        // Water layer (lowest)
        TerrainLayer water;
        water.name = "Water";
        water.brush_name = "sea";
        water.item_id = 4608;
        water.height_min = -1.0;
        water.height_max = 0.0;
        water.moisture_min = -1.0;
        water.moisture_max = 1.0;
        water.noise_scale = 1.0;
        water.coverage = 1.0;
        water.use_borders = true;
        water.z_order = 6000;
        terrain_layers.push_back(water);
        
        // Grass layer (main land)
        TerrainLayer grass;
        grass.name = "Grass";
        grass.brush_name = "grass";
        grass.item_id = 4526;
        grass.height_min = 0.0;
        grass.height_max = 0.7;
        grass.moisture_min = -0.5;
        grass.moisture_max = 1.0;
        grass.noise_scale = 1.0;
        grass.coverage = 1.0;
        grass.use_borders = true;
        grass.z_order = 3500;
        terrain_layers.push_back(grass);
        
        // Sand layer (dry areas)
        TerrainLayer sand;
        sand.name = "Sand";
        sand.brush_name = "sand";
        sand.item_id = 231;
        sand.height_min = 0.0;
        sand.height_max = 0.6;
        sand.moisture_min = -1.0;
        sand.moisture_max = -0.6;
        sand.noise_scale = 1.5;
        sand.coverage = 1.0;
        sand.use_borders = true;
        sand.z_order = 3400;
        sand.enabled = sand_biome;
        terrain_layers.push_back(sand);
        
        // Mountain layer (high elevation)
        TerrainLayer mountain;
        mountain.name = "Mountain";
        mountain.brush_name = "mountain";
        mountain.item_id = 919;  // From grounds.xml mountain brush
        mountain.height_min = 0.7;
        mountain.height_max = 1.0;
        mountain.moisture_min = -1.0;
        mountain.moisture_max = 1.0;
        mountain.noise_scale = 0.8;
        mountain.coverage = 1.0;
        mountain.use_borders = true;
        mountain.z_order = 9900;
        terrain_layers.push_back(mountain);
    }
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
    std::vector<std::vector<double>> generateMoistureMap(const GenerationConfig& config);
    std::vector<std::vector<uint16_t>> generateTerrainLayer(const std::vector<std::vector<double>>& heightMap, 
                                                          const std::vector<std::vector<double>>& moistureMap,
                                                          const GenerationConfig& config);
    
    // Helper methods for multi-floor generation
    void fillColumn(std::vector<std::vector<std::vector<uint16_t>>>& layers, 
                   int x, int y, int elevation, uint16_t surfaceTileId, 
                   const GenerationConfig& config);
    uint16_t getTerrainTileId(double height, double moisture, const GenerationConfig& config);
    const TerrainLayer* selectTerrainLayer(double height, double moisture, const GenerationConfig& config);
    
private:
    SimplexNoise* noise_generator;
    std::mt19937 rng;
    
    // Cave generation
    std::vector<std::vector<uint16_t>> generateCaveLayer(const GenerationConfig& config);
    
    // Utility functions
    double getDistance(int x, int y, int centerX, int centerY, bool euclidean = false);
    double smoothstep(double edge0, double edge1, double x);
    void seedRandom(const std::string& seed);
    
    // Border generation (integrate with brush system)
    void generateBorders(BaseMap* map, const GenerationConfig& config);
    void addBordersToTile(BaseMap* map, Tile* tile, int x, int y, int z);
    
    // Decoration placement
    void addClutter(BaseMap* map, const GenerationConfig& config);
    void placeTreesAndVegetation(BaseMap* map, Tile* tile, uint16_t groundId);
    void placeStones(BaseMap* map, Tile* tile, uint16_t groundId);
    void placeCaveDecorations(BaseMap* map, Tile* tile);
};

// Helper functions for tile creation and manipulation
namespace OTMapGenUtils {
    Tile* getOrCreateTile(BaseMap* map, int x, int y, int z);
    void setGroundTile(Tile* tile, uint16_t itemId);
    void addDecoration(Tile* tile, uint16_t itemId);
    bool isWaterTile(uint16_t itemId);
    bool isLandTile(uint16_t itemId);
    bool isMountainTile(uint16_t itemId);
    
    // Brush system integration
    std::vector<std::string> getAvailableBrushes();
    uint16_t getPrimaryItemFromBrush(const std::string& brushName);
    bool applyBrushToTile(BaseMap* map, Tile* tile, const std::string& brushName, int x, int y, int z);
}

#endif 