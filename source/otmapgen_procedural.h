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

#ifndef RME_OTMAPGEN_PROCEDURAL_H_
#define RME_OTMAPGEN_PROCEDURAL_H_

#include "otmapgen_base.h"

class Map;

// Procedural Configuration
struct ProceduralConfig {
    // Target location
    int target_floor;
    
    // Generation settings
    std::string seed;
    int width;
    int height;
    
    // Terrain layers
    std::vector<TerrainLayer> terrain_layers;
    
    // Advanced noise settings
    double noise_increment;
    double exponent;
    double linear_multiplier;
    double island_distance_exponent;
    double island_distance_decrement;
    bool euclidean_distance;
    
    // Water settings
    uint16_t water_id;
    int water_level;
    
    // Cave generation
    bool add_caves;
    int cave_depth;
    double cave_roughness;
    double cave_chance;
    uint16_t cave_wall_id;
    
    // Multi-floor settings
    int floors_to_generate;
    bool terrain_only;
    
    // Frequency weights for noise
    std::vector<FrequencyWeight> frequencies;
    
    ProceduralConfig() :
        target_floor(7), seed("procedural"), width(100), height(100),
        noise_increment(0.01), exponent(1.0), linear_multiplier(1.0),
        island_distance_exponent(2.0), island_distance_decrement(1.0), euclidean_distance(true),
        water_id(4608), water_level(7),
        add_caves(false), cave_depth(3), cave_roughness(0.05), cave_chance(0.1), cave_wall_id(351),
        floors_to_generate(1), terrain_only(false) {}
};

// Procedural Map Generator
class OTMapGeneratorProcedural : public OTMapGeneratorBase {
public:
    OTMapGeneratorProcedural();
    virtual ~OTMapGeneratorProcedural();
    
    // Main generation methods
    bool generateProceduralMap(Map* map, const ProceduralConfig& config);
    std::vector<std::vector<uint16_t>> generateTerrainLayer(const ProceduralConfig& config, int floor);
    
private:
    // Terrain generation
    std::vector<std::vector<double>> generateNoiseMap(const ProceduralConfig& config);
    std::vector<std::vector<double>> generateMoistureMap(const ProceduralConfig& config);
    std::vector<std::vector<double>> applyIslandMask(const std::vector<std::vector<double>>& heightMap, const ProceduralConfig& config);
    
    // Cave generation
    void generateCaves(Map* map, const ProceduralConfig& config, int floor);
    bool shouldPlaceCave(double height, double noise, const ProceduralConfig& config);
    void placeCaveTile(Map* map, int x, int y, int z, const ProceduralConfig& config);
    
    // Multi-floor generation
    void generateMultipleFloors(Map* map, const ProceduralConfig& config);
    void generateFloor(Map* map, const ProceduralConfig& config, int floor);
    
    // Terrain selection
    const TerrainLayer* selectBestTerrainLayer(double height, double moisture, const ProceduralConfig& config);
    uint16_t getTerrainTileId(double height, double moisture, const ProceduralConfig& config);
    
    // Post-processing
    void applyTerrainSmoothing(Map* map, const ProceduralConfig& config, int floor);
    void addTerrainTransitions(Map* map, const ProceduralConfig& config, int floor);
    
    // Utility methods
    double calculateIslandDistance(int x, int y, const ProceduralConfig& config);
    double applyDistanceFalloff(double distance, const ProceduralConfig& config);
    bool isValidTerrainPlacement(const TerrainLayer& layer, double height, double moisture);
};

#endif 