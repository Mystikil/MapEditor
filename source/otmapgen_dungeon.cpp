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
#include "otmapgen_dungeon.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include <algorithm>
#include <queue>
#include <set>

OTMapGeneratorDungeon::OTMapGeneratorDungeon() : OTMapGeneratorBase() {
}

OTMapGeneratorDungeon::~OTMapGeneratorDungeon() {
}

bool OTMapGeneratorDungeon::generateDungeonMap(Map* map, const DungeonConfig& config, int width, int height, const std::string& seed) {
    if (!map) return false;
    
    seedRandom(seed);
    
    // Generate dungeon grid
    std::vector<std::vector<int>> grid(height, std::vector<int>(width, 0)); // 0 = wall, 1 = floor
    
    // Generate rooms
    std::vector<Room> rooms = generateRooms(config, width, height);
    
    // Place rooms on grid
    for (const auto& room : rooms) {
        placeRoom(grid, room, config);
    }
    
    // Generate intersections if enabled
    std::vector<Intersection> intersections;
    if (config.add_triple_intersections || config.add_quad_intersections) {
        intersections = generateIntersections(config, rooms, width, height);
        for (const auto& intersection : intersections) {
            placeIntersection(grid, intersection, config);
        }
    }
    
    // Generate corridors
    generateCorridors(grid, rooms, config, width, height);
    
    // Connect rooms via intersections if available
    if (!intersections.empty()) {
        connectRoomsViaIntersections(grid, rooms, intersections, config, width, height);
    }
    
    // Add dead ends if requested
    if (config.add_dead_ends) {
        addDeadEnds(grid, config, width, height);
    }
    
    // Convert grid to actual map tiles
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Position pos(x, y, config.target_floor);
            Tile* tile = map->getTile(pos);
            if (!tile) {
                tile = map->allocator(map->createTileL(pos));
                map->setTile(pos, tile);
            }
            
            // Determine tile type
            uint16_t tileId;
            if (grid[y][x] == 1) {
                tileId = config.ground_id; // Floor
            } else {
                tileId = config.wall_id; // Wall
            }
            
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
    
    return true;
}

std::vector<Room> OTMapGeneratorDungeon::generateRooms(const DungeonConfig& config, int width, int height) {
    std::vector<Room> rooms;
    rooms.reserve(config.room_count);
    
    // Generate rooms with random placement
    for (int i = 0; i < config.room_count; ++i) {
        Room room;
        
        // Random size within bounds
        room.width = rng() % (config.room_max_size - config.room_min_size + 1) + config.room_min_size;
        room.height = rng() % (config.room_max_size - config.room_min_size + 1) + config.room_min_size;
        
        // Random position ensuring room fits in map
        room.x = rng() % (width - room.width - 2) + 1;
        room.y = rng() % (height - room.height - 2) + 1;
        
        room.isCircular = config.circular_rooms;
        if (room.isCircular) {
            room.radius = std::min(room.width, room.height) / 2;
        }
        
        // Check for overlap with existing rooms
        bool overlaps = false;
        for (const auto& existingRoom : rooms) {
            if (room.x < existingRoom.x + existingRoom.width + 2 &&
                room.x + room.width + 2 > existingRoom.x &&
                room.y < existingRoom.y + existingRoom.height + 2 &&
                room.y + room.height + 2 > existingRoom.y) {
                overlaps = true;
                break;
            }
        }
        
        if (!overlaps) {
            rooms.push_back(room);
        }
    }
    
    return rooms;
}

void OTMapGeneratorDungeon::placeRoom(std::vector<std::vector<int>>& grid, const Room& room, const DungeonConfig& config) {
    if (room.isCircular) {
        // Place circular room
        int centerX = room.x + room.width / 2;
        int centerY = room.y + room.height / 2;
        
        for (int y = room.y; y < room.y + room.height; ++y) {
            for (int x = room.x; x < room.x + room.width; ++x) {
                double distance = sqrt(pow(x - centerX, 2) + pow(y - centerY, 2));
                if (distance <= room.radius) {
                    grid[y][x] = 1; // Floor
                }
            }
        }
    } else {
        // Place rectangular room
        for (int y = room.y; y < room.y + room.height; ++y) {
            for (int x = room.x; x < room.x + room.width; ++x) {
                grid[y][x] = 1; // Floor
            }
        }
    }
}

void OTMapGeneratorDungeon::generateCorridors(std::vector<std::vector<int>>& grid, const std::vector<Room>& rooms, const DungeonConfig& config, int width, int height) {
    // Connect each room to the next one
    for (size_t i = 0; i < rooms.size() - 1; ++i) {
        const Room& room1 = rooms[i];
        const Room& room2 = rooms[i + 1];
        
        // Find center points of rooms
        int x1 = room1.x + room1.width / 2;
        int y1 = room1.y + room1.height / 2;
        int x2 = room2.x + room2.width / 2;
        int y2 = room2.y + room2.height / 2;
        
        // Use smart corridor if enabled, otherwise use basic corridor
        if (config.use_smart_pathfinding) {
            createSmartCorridor(grid, x1, y1, x2, y2, config, width, height);
        } else {
            createCorridor(grid, x1, y1, x2, y2, config, width, height);
        }
    }
    
    // Additional corridors for complexity
    int extraCorridors = static_cast<int>(config.complexity * rooms.size());
    for (int i = 0; i < extraCorridors && i < config.corridor_count; ++i) {
        if (rooms.size() >= 2) {
            size_t roomIndex1 = rng() % rooms.size();
            size_t roomIndex2 = rng() % rooms.size();
            
            if (roomIndex1 != roomIndex2) {
                const Room& room1 = rooms[roomIndex1];
                const Room& room2 = rooms[roomIndex2];
                
                int x1 = room1.x + room1.width / 2;
                int y1 = room1.y + room1.height / 2;
                int x2 = room2.x + room2.width / 2;
                int y2 = room2.y + room2.height / 2;
                
                if (config.use_smart_pathfinding) {
                    createSmartCorridor(grid, x1, y1, x2, y2, config, width, height);
                } else {
                    createCorridor(grid, x1, y1, x2, y2, config, width, height);
                }
            }
        }
    }
}

void OTMapGeneratorDungeon::createCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height) {
    int currentX = x1;
    int currentY = y1;
    
    // Horizontal segment only - no vertical movement as requested
    while (currentX != x2) {
        // Create corridor with specified width
        for (int w = 0; w < config.corridor_width; ++w) {
            for (int h = 0; h < config.corridor_width; ++h) {
                int px = currentX + w - config.corridor_width / 2;
                int py = currentY + h - config.corridor_width / 2;
                
                if (px >= 0 && px < width && py >= 0 && py < height) {
                    grid[py][px] = 1; // Mark as corridor
                }
            }
        }
        
        currentX += (x2 > x1) ? 1 : -1;
    }
}

void OTMapGeneratorDungeon::createSmartCorridor(std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, const DungeonConfig& config, int width, int height) {
    // Calculate distance
    int distance = abs(x2 - x1) + abs(y2 - y1);
    
    // If distance exceeds max length, create segments
    if (distance > config.max_corridor_length) {
        // Find shortest path if available
        std::vector<std::pair<int, int>> path = findShortestPath(grid, x1, y1, x2, y2, width, height, config.max_corridor_length);
        
        if (!path.empty()) {
            createCorridorSegments(grid, path, config, width, height);
        } else {
            // Fallback: create waypoint-based segments
            int segments = (distance / config.max_corridor_length) + 1;
            int segmentLength = distance / segments;
            
            int currentX = x1;
            int currentY = y1;
            
            for (int i = 0; i < segments; ++i) {
                int targetX = (i == segments - 1) ? x2 : currentX + ((x2 - currentX) * segmentLength / distance);
                int targetY = currentY; // Keep Y constant for horizontal-only movement
                
                createCorridor(grid, currentX, currentY, targetX, targetY, config, width, height);
                currentX = targetX;
                currentY = targetY;
            }
        }
    } else {
        // Direct corridor for short distances
        createCorridor(grid, x1, y1, x2, y2, config, width, height);
    }
}

std::vector<std::pair<int, int>> OTMapGeneratorDungeon::findShortestPath(const std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, int width, int height, int maxLength) {
    // Simple A* pathfinding implementation
    struct Node {
        int x, y;
        int g, h, f;
        std::pair<int, int> parent;
        
        Node(int x, int y, int g, int h, std::pair<int, int> parent) 
            : x(x), y(y), g(g), h(h), f(g + h), parent(parent) {}
        
        bool operator<(const Node& other) const {
            return f > other.f; // For min-heap
        }
    };
    
    std::priority_queue<Node> openList;
    std::set<std::pair<int, int>> closedList;
    std::map<std::pair<int, int>, std::pair<int, int>> parentMap;
    
    openList.push(Node(x1, y1, 0, abs(x2 - x1) + abs(y2 - y1), {-1, -1}));
    
    const int dx[] = {-1, 1, 0, 0}; // Only horizontal movement for corridors
    const int dy[] = {0, 0, 0, 0};  // No vertical movement
    
    while (!openList.empty()) {
        Node current = openList.top();
        openList.pop();
        
        if (current.x == x2 && current.y == y2) {
            // Reconstruct path
            std::vector<std::pair<int, int>> path;
            std::pair<int, int> pos = {current.x, current.y};
            
            while (pos.first != -1 && pos.second != -1) {
                path.push_back(pos);
                auto it = parentMap.find(pos);
                if (it != parentMap.end()) {
                    pos = it->second;
                } else {
                    break;
                }
            }
            
            std::reverse(path.begin(), path.end());
            return path;
        }
        
        closedList.insert({current.x, current.y});
        
        // Check adjacent cells (only horizontal for corridors)
        for (int i = 0; i < 2; ++i) { // Only check left and right
            int nx = current.x + dx[i];
            int ny = current.y + dy[i];
            
            if (nx >= 0 && nx < width && ny >= 0 && ny < height &&
                closedList.find({nx, ny}) == closedList.end()) {
                
                int newG = current.g + 1;
                if (newG <= maxLength) {
                    int h = abs(x2 - nx) + abs(y2 - ny);
                    openList.push(Node(nx, ny, newG, h, {current.x, current.y}));
                    parentMap[{nx, ny}] = {current.x, current.y};
                }
            }
        }
    }
    
    return {}; // No path found
}

void OTMapGeneratorDungeon::createCorridorSegments(std::vector<std::vector<int>>& grid, const std::vector<std::pair<int, int>>& path, const DungeonConfig& config, int width, int height) {
    for (size_t i = 0; i < path.size() - 1; ++i) {
        createCorridor(grid, path[i].first, path[i].second, path[i + 1].first, path[i + 1].second, config, width, height);
    }
}

std::vector<Intersection> OTMapGeneratorDungeon::generateIntersections(const DungeonConfig& config, const std::vector<Room>& rooms, int width, int height) {
    std::vector<Intersection> intersections;
    
    if (!config.add_triple_intersections && !config.add_quad_intersections) {
        return intersections;
    }
    
    // Generate intersections based on configuration
    for (int i = 0; i < config.intersection_count; ++i) {
        Intersection intersection;
        
        // Random position ensuring intersection fits
        int margin = config.intersection_size + 2;
        intersection.centerX = rng() % (width - 2 * margin) + margin;
        intersection.centerY = rng() % (height - 2 * margin) + margin;
        intersection.size = config.intersection_size;
        
        // Determine connection count
        if (config.add_quad_intersections && (rng() % 100) < (config.intersection_probability * 100)) {
            intersection.connectionCount = 4; // Quadruple intersection
        } else if (config.add_triple_intersections) {
            intersection.connectionCount = 3; // Triple intersection
        } else {
            continue; // Skip if no valid type
        }
        
        // Check for overlap with rooms
        bool overlaps = false;
        for (const auto& room : rooms) {
            if (intersection.centerX - intersection.size < room.x + room.width &&
                intersection.centerX + intersection.size > room.x &&
                intersection.centerY - intersection.size < room.y + room.height &&
                intersection.centerY + intersection.size > room.y) {
                overlaps = true;
                break;
            }
        }
        
        // Check spacing with other intersections
        for (const auto& existing : intersections) {
            int distance = abs(intersection.centerX - existing.centerX) + abs(intersection.centerY - existing.centerY);
            if (distance < (intersection.size + existing.size + 4)) {
                overlaps = true;
                break;
            }
        }
        
        if (!overlaps) {
            intersections.push_back(intersection);
        }
    }
    
    return intersections;
}

void OTMapGeneratorDungeon::placeIntersection(std::vector<std::vector<int>>& grid, const Intersection& intersection, const DungeonConfig& config) {
    // Create intersection area
    for (int y = intersection.centerY - intersection.size; y <= intersection.centerY + intersection.size; ++y) {
        for (int x = intersection.centerX - intersection.size; x <= intersection.centerX + intersection.size; ++x) {
            if (x >= 0 && x < static_cast<int>(grid[0].size()) && y >= 0 && y < static_cast<int>(grid.size())) {
                grid[y][x] = 1; // Floor
            }
        }
    }
}

void OTMapGeneratorDungeon::connectRoomsViaIntersections(std::vector<std::vector<int>>& grid, const std::vector<Room>& rooms, const std::vector<Intersection>& intersections, const DungeonConfig& config, int width, int height) {
    // Connect some rooms to nearby intersections
    for (const auto& room : rooms) {
        std::pair<int, int> nearest = findNearestIntersection(intersections, room.x + room.width / 2, room.y + room.height / 2);
        
        if (nearest.first != -1 && nearest.second != -1) {
            if (config.prefer_intersections && findIntersectionPath(grid, room.x + room.width / 2, room.y + room.height / 2, nearest.first, nearest.second, width, height)) {
                createCorridor(grid, room.x + room.width / 2, room.y + room.height / 2, nearest.first, nearest.second, config, width, height);
            }
        }
    }
}

std::pair<int, int> OTMapGeneratorDungeon::findNearestIntersection(const std::vector<Intersection>& intersections, int x, int y) {
    int minDistance = INT_MAX;
    std::pair<int, int> nearest = {-1, -1};
    
    for (const auto& intersection : intersections) {
        int distance = abs(intersection.centerX - x) + abs(intersection.centerY - y);
        if (distance < minDistance) {
            minDistance = distance;
            nearest = {intersection.centerX, intersection.centerY};
        }
    }
    
    return nearest;
}

bool OTMapGeneratorDungeon::findIntersectionPath(const std::vector<std::vector<int>>& grid, int x1, int y1, int x2, int y2, int width, int height) {
    // Simple path validation - check if there's a reasonable path
    int distance = abs(x2 - x1) + abs(y2 - y1);
    return distance < width / 4; // Only connect if reasonably close
}

void OTMapGeneratorDungeon::addDeadEnds(std::vector<std::vector<int>>& grid, const DungeonConfig& config, int width, int height) {
    // Add some random dead-end corridors for variety
    int deadEndCount = static_cast<int>(config.complexity * 5);
    
    for (int i = 0; i < deadEndCount; ++i) {
        // Find a random floor tile to extend from
        int x = rng() % width;
        int y = rng() % height;
        
        if (grid[y][x] == 1) { // If it's a floor tile
            // Create a short dead-end corridor
            int length = rng() % 5 + 3;
            int direction = rng() % 2; // 0 = horizontal left, 1 = horizontal right
            
            for (int j = 1; j <= length; ++j) {
                int nx = x + (direction == 0 ? -j : j);
                int ny = y;
                
                if (nx >= 0 && nx < width && ny >= 0 && ny < height && grid[ny][nx] == 0) {
                    grid[ny][nx] = 1; // Floor
                } else {
                    break; // Stop if we hit existing floor or boundary
                }
            }
        }
    }
}

bool OTMapGeneratorDungeon::isWallPosition(const std::vector<std::vector<int>>& grid, int x, int y, int width, int height) {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return true; // Boundaries are walls
    }
    return grid[y][x] == 0;
} 