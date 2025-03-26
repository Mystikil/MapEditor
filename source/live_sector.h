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

#ifndef _RME_LIVE_SECTOR_H_
#define _RME_LIVE_SECTOR_H_

#include "position.h"
#include "live_packets.h"
#include <chrono>
#include <unordered_map>
#include <mutex>

// A struct to identify a sector
struct SectorCoord {
    int x;
    int y;
    
    // Default constructor (needed for some STL containers)
    SectorCoord() : x(0), y(0) {}
    
    SectorCoord(int _x, int _y) : x(_x), y(_y) {}
    
    bool operator==(const SectorCoord& other) const {
        return x == other.x && y == other.y;
    }
    
    // Less than operator for ordering in sets
    bool operator<(const SectorCoord& other) const {
        if (x != other.x)
            return x < other.x;
        return y < other.y;
    }
};

// Hash function for SectorCoord to use in unordered_map
namespace std {
    template<>
    struct hash<SectorCoord> {
        size_t operator()(const SectorCoord& s) const {
            return (std::hash<int>()(s.x) ^ (std::hash<int>()(s.y) << 1));
        }
    };
}

// Tracks information about a sector edit
struct SectorLock {
    uint32_t clientId;
    std::chrono::steady_clock::time_point lockTime;
    uint32_t version;
    bool dirty;
    
    SectorLock(uint32_t _clientId) : 
        clientId(_clientId), 
        lockTime(std::chrono::steady_clock::now()),
        version(1),
        dirty(false) {}
};

class LiveSectorManager {
public:
    LiveSectorManager();
    ~LiveSectorManager();
    
    // Convert a map position to sector coordinates
    static SectorCoord positionToSector(int x, int y) {
        return SectorCoord(x / SECTOR_SIZE, y / SECTOR_SIZE);
    }
    
    // Convert sector coordinates to the top-left position in the map
    static Position sectorToPosition(const SectorCoord& sector) {
        // Create a Position with explicit coordinates
        Position pos;
        pos.x = sector.x * SECTOR_SIZE;
        pos.y = sector.y * SECTOR_SIZE;
        pos.z = 0;
        return pos;
    }
    
    // Request a lock on a sector for a client
    bool requestLock(uint32_t clientId, const SectorCoord& sector);
    
    // Release a lock on a sector
    void releaseLock(uint32_t clientId, const SectorCoord& sector);
    
    // Check if a client has a lock on a sector
    bool hasLock(uint32_t clientId, const SectorCoord& sector) const;
    
    // Check which client has a lock on a sector (0 if no lock)
    uint32_t getLockOwner(const SectorCoord& sector) const;
    
    // Mark a sector as having changes
    void markDirty(const SectorCoord& sector);
    
    // Check if a sector has changes
    bool isDirty(const SectorCoord& sector) const;
    
    // Get the current version of a sector
    uint32_t getSectorVersion(const SectorCoord& sector) const;
    
    // Increment the version of a sector
    uint32_t incrementSectorVersion(const SectorCoord& sector);
    
    // Get the set of nodes that belong to a sector
    std::vector<Position> getSectorNodes(const SectorCoord& sector) const {
        std::vector<Position> nodes;
        
        // Calculate the boundaries of this sector
        int startX = sector.x * SECTOR_SIZE;
        int startY = sector.y * SECTOR_SIZE;
        
        // QTreeNode works on 4x4 tile groups
        // So we need to find all QTreeNodes contained in this sector
        for (int x = 0; x < SECTOR_SIZE; x += 4) {
            for (int y = 0; y < SECTOR_SIZE; y += 4) {
                // Each position corresponds to a QTreeNode leaf
                nodes.push_back(Position(startX + x, startY + y, 0));
            }
        }
        
        return nodes;
    }
    
    // Clear all locks (useful on server shutdown)
    void clearAllLocks();
    
    // Timeout old locks (e.g., from disconnected clients)
    void timeoutOldLocks(std::chrono::seconds maxAge);
    
private:
    std::unordered_map<SectorCoord, SectorLock> sectorLocks;
    mutable std::mutex lockMutex;
};

#endif  