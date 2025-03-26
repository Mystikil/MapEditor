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
#include "live_sector.h"

LiveSectorManager::LiveSectorManager() {
    // Nothing to initialize
}

LiveSectorManager::~LiveSectorManager() {
    // Clean up
    clearAllLocks();
}

bool LiveSectorManager::requestLock(uint32_t clientId, const SectorCoord& sector) {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    // Check if this sector is already locked
    auto it = sectorLocks.find(sector);
    if (it != sectorLocks.end()) {
        // If it's locked by the same client, it's fine
        if (it->second.clientId == clientId) {
            // Refresh the lock time
            it->second.lockTime = std::chrono::steady_clock::now();
            return true;
        }
        // Otherwise, it's locked by someone else
        return false;
    }
    
    // Create a new lock
    sectorLocks.emplace(sector, SectorLock(clientId));
    return true;
}

void LiveSectorManager::releaseLock(uint32_t clientId, const SectorCoord& sector) {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    auto it = sectorLocks.find(sector);
    if (it != sectorLocks.end() && it->second.clientId == clientId) {
        sectorLocks.erase(it);
    }
}

bool LiveSectorManager::hasLock(uint32_t clientId, const SectorCoord& sector) const {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    auto it = sectorLocks.find(sector);
    return (it != sectorLocks.end() && it->second.clientId == clientId);
}

uint32_t LiveSectorManager::getLockOwner(const SectorCoord& sector) const {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    auto it = sectorLocks.find(sector);
    return (it != sectorLocks.end()) ? it->second.clientId : 0;
}

void LiveSectorManager::markDirty(const SectorCoord& sector) {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    auto it = sectorLocks.find(sector);
    if (it != sectorLocks.end()) {
        it->second.dirty = true;
    }
}

bool LiveSectorManager::isDirty(const SectorCoord& sector) const {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    auto it = sectorLocks.find(sector);
    return (it != sectorLocks.end() && it->second.dirty);
}

uint32_t LiveSectorManager::getSectorVersion(const SectorCoord& sector) const {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    auto it = sectorLocks.find(sector);
    return (it != sectorLocks.end()) ? it->second.version : 0;
}

uint32_t LiveSectorManager::incrementSectorVersion(const SectorCoord& sector) {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    auto it = sectorLocks.find(sector);
    if (it != sectorLocks.end()) {
        return ++it->second.version;
    }
    
    // Create a new lock entry with version 1
    auto result = sectorLocks.emplace(sector, SectorLock(0));
    return result.first->second.version;
}

void LiveSectorManager::clearAllLocks() {
    std::lock_guard<std::mutex> guard(lockMutex);
    sectorLocks.clear();
}

void LiveSectorManager::timeoutOldLocks(std::chrono::seconds maxAge) {
    std::lock_guard<std::mutex> guard(lockMutex);
    
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = sectorLocks.begin(); it != sectorLocks.end();) {
        auto lockDuration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lockTime);
        
        if (lockDuration > maxAge) {
            // This lock has expired
            it = sectorLocks.erase(it);
        } else {
            ++it;
        }
    }
} 