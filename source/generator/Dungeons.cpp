#include "Dungeons.h"

#include "MapGenerator.h"

#include "item.h"
#include "map.h"
#include "tile.h"

#include <algorithm>
#include <random>
#include <vector>

namespace Dungeons {
namespace {

Tile* ensureTile(Map& map, int x, int y, int z) {
        Tile* tile = map.getTile(x, y, z);
        if (!tile) {
                tile = map.allocator.allocateTile(map.createTileL(x, y, z));
                map.setTile(x, y, z, tile, true);
        }
        return tile;
}

void setGround(Tile* tile, uint16_t id) {
        if (tile->ground) {
                delete tile->ground;
                tile->ground = nullptr;
        }
        for (Item* item : tile->items) {
                delete item;
        }
        tile->items.clear();
        tile->addItem(Item::Create(id));
}

struct Room {
        int x;
        int y;
        int w;
        int h;

        int centerX() const { return x + w / 2; }
        int centerY() const { return y + h / 2; }
};

bool intersects(const Room& a, const Room& b) {
        return !(a.x + a.w + 2 < b.x || b.x + b.w + 2 < a.x || a.y + a.h + 2 < b.y || b.y + b.h + 2 < a.y);
}

void carveRoom(Map& map, const Room& room, const Biomes::BiomePreset& preset, int z) {
        for (int y = room.y; y < room.y + room.h; ++y) {
                for (int x = room.x; x < room.x + room.w; ++x) {
                        Tile* tile = ensureTile(map, x, y, z);
                        const bool border = (x == room.x) || (y == room.y) || (x == room.x + room.w - 1) || (y == room.y + room.h - 1);
                        if (border) {
                                setGround(tile, preset.tiles.rock);
                        } else {
                                setGround(tile, preset.tiles.dirt);
                        }
                }
        }
}

void carveCorridor(Map& map, int x1, int y1, int x2, int y2, const Biomes::BiomePreset& preset, int z) {
        int x = x1;
        int y = y1;
        while (x != x2) {
                Tile* tile = ensureTile(map, x, y, z);
                setGround(tile, preset.tiles.dirt);
                x += (x2 > x) ? 1 : -1;
        }
        while (y != y2) {
                Tile* tile = ensureTile(map, x, y, z);
                setGround(tile, preset.tiles.dirt);
                y += (y2 > y) ? 1 : -1;
        }
        Tile* tile = ensureTile(map, x, y, z);
        setGround(tile, preset.tiles.dirt);
}

} // namespace

void Generate(Map& map,
              const Biomes::BiomePreset& preset,
              const GenOptions& options,
              std::mt19937_64& rng) {
        if (options.width <= 0 || options.height <= 0) {
                return;
        }

        const int z = options.caveZ + 1;
        const int width = options.width;
        const int height = options.height;

        const int targetRooms = std::max(5, (width * height) / 65536 + 3);
        std::uniform_int_distribution<int> roomWidth(6, 12);
        std::uniform_int_distribution<int> roomHeight(6, 12);
        std::uniform_int_distribution<int> roomX(1, std::max(2, width - 14));
        std::uniform_int_distribution<int> roomY(1, std::max(2, height - 14));

        std::vector<Room> rooms;
        for (int attempt = 0; attempt < targetRooms * 8 && static_cast<int>(rooms.size()) < targetRooms; ++attempt) {
                Room room;
                room.w = roomWidth(rng);
                room.h = roomHeight(rng);
                room.x = std::min(std::max(1, roomX(rng)), width - room.w - 1);
                room.y = std::min(std::max(1, roomY(rng)), height - room.h - 1);

                bool overlaps = false;
                for (const Room& other : rooms) {
                        if (intersects(room, other)) {
                                overlaps = true;
                                break;
                        }
                }
                if (!overlaps) {
                        rooms.push_back(room);
                        carveRoom(map, room, preset, z);
                }
        }

        if (rooms.empty()) {
                return;
        }

        std::sort(rooms.begin(), rooms.end(), [](const Room& a, const Room& b) {
                return a.centerX() < b.centerX();
        });

        for (size_t i = 1; i < rooms.size(); ++i) {
                const Room& prev = rooms[i - 1];
                const Room& current = rooms[i];
                carveCorridor(map, prev.centerX(), prev.centerY(), current.centerX(), current.centerY(), preset, z);
        }
}

} // namespace Dungeons

