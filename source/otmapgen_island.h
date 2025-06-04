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

#ifndef RME_OTMAPGEN_ISLAND_H_
#define RME_OTMAPGEN_ISLAND_H_

#include "otmapgen_base.h"

// Island configuration
struct IslandConfig {
    // Basic settings
    int target_floor;
    uint16_t water_id;
    uint16_t ground_id;
    
    // Island shape settings
    double island_size;
    double island_falloff;
    double island_threshold;
    double water_level;
    
    // Noise settings
    double noise_scale;
    double noise_persistence;
    double noise_lacunarity;
    int noise_octaves;
    
    // Cleanup settings
    bool enable_cleanup;
    int smoothing_passes;
    int min_land_patch_size;
    int max_water_hole_size;
    
    IslandConfig() :
        target_floor(7), water_id(4608), ground_id(4526),
        island_size(0.8), island_falloff(2.0), island_threshold(0.3), water_level(0.0),
        noise_scale(0.01), noise_persistence(0.5), noise_lacunarity(2.0), noise_octaves(6),
        enable_cleanup(true), smoothing_passes(2), min_land_patch_size(5), max_water_hole_size(3) {}
};

// Island Map Generator
class OTMapGeneratorIsland : public OTMapGeneratorBase {
public:
    OTMapGeneratorIsland();
    virtual ~OTMapGeneratorIsland();
    
    // Main generation methods
    bool generateIslandMap(BaseMap* map, const IslandConfig& config, int width, int height, const std::string& seed);
    std::vector<std::vector<uint16_t>> generateIslandLayer(const IslandConfig& config, int width, int height, const std::string& seed);
    std::vector<std::vector<uint16_t>> generateIslandLayerBatch(const IslandConfig& config, int width, int height, const std::string& seed, int offsetX, int offsetY, int totalWidth, int totalHeight);
    
private:
    // Island-specific generation
    std::vector<std::vector<double>> generateIslandHeightMap(const IslandConfig& config, int width, int height, const std::string& seed);
    double generateIslandNoise(double x, double y, const IslandConfig& config);
    double getIslandDistance(int x, int y, int centerX, int centerY, double island_size);
    double applyIslandFalloff(double distance, double falloff);
    
    // Cleanup and post-processing
    void cleanupIslandTerrain(std::vector<std::vector<uint16_t>>& terrainLayer, const IslandConfig& config, int width, int height);
    void removeIsolatedPixels(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config, uint16_t target_id);
    void removeSmallPatches(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config, uint16_t target_id, int min_size);
    void fillSmallHoles(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, uint16_t target_id, uint16_t fill_id, int max_hole_size);
    void smoothTerrain(std::vector<std::vector<uint16_t>>& terrainLayer, int width, int height, const IslandConfig& config);
    int floodFillCount(std::vector<std::vector<uint16_t>>& terrainLayer, int x, int y, int width, int height, uint16_t target_id, uint16_t replacement_id);
};

#endif 