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

#ifndef RME_OTMAPGEN_DUNGEON_H_
#define RME_OTMAPGEN_DUNGEON_H_

#include "otmapgen_base.h"

// Dungeon configuration
struct DungeonConfig {
    // Room settings
    int room_count;
    int room_min_size;
    int room_max_size;
    bool circular_rooms;
    
    // Corridor settings
    int corridor_count;
    int corridor_width;
    double complexity;
    bool add_dead_ends;
    int max_corridor_length;
    bool use_smart_pathfinding;
    bool prefer_intersections;
    int corridor_segments;
    
    // Intersection settings
    bool add_triple_intersections;
    bool add_quad_intersections;
    int intersection_count;
    int intersection_size;
    double intersection_probability;
    
    // Tile IDs
    uint16_t ground_id;
    uint16_t wall_id;
    uint16_t fill_id;
    std::string wall_brush;
    
    DungeonConfig() :
        room_count(8), room_min_size(3), room_max_size(7), circular_rooms(false),
        corridor_count(10), corridor_width(1), complexity(0.3), add_dead_ends(true),
        max_corridor_length(50), use_smart_pathfinding(true), prefer_intersections(true), corridor_segments(4),
        add_triple_intersections(true), add_quad_intersections(false), intersection_count(3), intersection_size(2), intersection_probability(0.5),
        ground_id(4526), wall_id(1025), fill_id(919), wall_brush("brick wall") {}
};

// Room structure
struct Room {
    int centerX;
    int centerY;
    int radius;
    bool isCircular;
    
    Room() : centerX(0), centerY(0), radius(0), isCircular(false) {}
};

// Intersection structure
struct Intersection {
    int centerX;
    int centerY;
    int size;
    int connectionCount;
    std::vector<std::pair<int, int>> connections;
    
    Intersection() : centerX(0), centerY(0), size(0), connectionCount(0) {}
};

// Dungeon Map Generator
class OTMapGeneratorDungeon : public OTMapGeneratorBase {
public:
    OTMapGeneratorDungeon();
    virtual ~OTMapGeneratorDungeon();
    
    // Main generation methods
    bool generateDungeonMap(BaseMap* map, const DungeonConfig& config, int width, int height, const std::string& seed);
    std::vector<std::vector<uint16_t>> generateDungeonLayer(const DungeonConfig& config, int width, int height, const std::string& seed);
    
private:
    // Room generation
    std::vector<Room> generateRooms(const DungeonConfig& config, int width, int height);
    void placeRoom(std::vector<std::vector<int>>& grid, const Room& room, const DungeonConfig& config);
    
    // Corridor generation
    void generateCorridors(std::vector<std::vector<int>>& grid, const std::vector<Room>& rooms, const DungeonConfig& config, int width, int height);
    void createCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height);
    void createSmartCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height);
    
    // Smart pathfinding
    std::vector<std::pair<int, int>> findShortestPath(const std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, int width, int height, int maxLength);
    void createCorridorSegments(std::vector<std::vector<int>>& grid, const std::vector<std::pair<int, int>>& path, const DungeonConfig& config, int width, int height);
    std::pair<int, int> findNearestIntersection(const std::vector<Intersection>& intersections, int x, int y);
    
    // Intersection generation
    std::vector<Intersection> generateIntersections(const DungeonConfig& config, const std::vector<Room>& rooms, int width, int height);
    void placeIntersection(std::vector<std::vector<int>>& grid, const Intersection& intersection, const DungeonConfig& config);
    void connectRoomsViaIntersections(std::vector<std::vector<int>>& grid, const std::vector<Room>& rooms, const std::vector<Intersection>& intersections, const DungeonConfig& config, int width, int height);
    bool findIntersectionPath(const std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, int width, int height);
    
    // Dead ends and utility
    void addDeadEnds(std::vector<std::vector<int>>& grid, const DungeonConfig& config, int width, int height);
    bool isWallPosition(const std::vector<std::vector<int>>& grid, int x, int y, int width, int height);
};

#endif 