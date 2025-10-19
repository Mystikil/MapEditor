#include "Biomes.h"

#include "MapGenerator.h"

#include "item.h"
#include "map.h"
#include "position.h"
#include "tile.h"

#include "json/json_spirit_reader.h"
#include "json/json_spirit_utils.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include <wx/dir.h>
#include <wx/filename.h>

namespace Biomes {
namespace {

std::string presetsDirectory(const std::string& dataDirectory) {
        wxFileName root(wxString::FromUTF8(dataDirectory.c_str()));
        root.AppendDir("generator");
        root.AppendDir("biomes");
        return root.GetFullPath().ToStdString();
}

uint16_t readTileId(const json_spirit::Object& obj, const char* key) {
        json_spirit::Value val = json_spirit::find_value(obj, key);
        if (val.type() != json_spirit::int_type) {
                throw std::runtime_error(std::string("Missing or invalid tile id for ") + key);
        }
        int id = val.get_int();
        if (id < 0 || id > 0xFFFF) {
                throw std::runtime_error(std::string("Tile id out of range for ") + key);
        }
        return static_cast<uint16_t>(id);
}

void clearTile(Tile* tile) {
        if (!tile) {
                return;
        }
        if (tile->ground) {
                delete tile->ground;
                tile->ground = nullptr;
        }
        for (Item* item : tile->items) {
                delete item;
        }
        tile->items.clear();
}

Tile* obtainTile(Map& map, int x, int y, int z) {
        Tile* tile = map.getTile(x, y, z);
        if (!tile) {
                tile = map.allocator.allocateTile(map.createTileL(x, y, z));
                map.setTile(x, y, z, tile, true);
        }
        return tile;
}

} // namespace

std::vector<std::string> ListPresets(const std::string& dataDirectory) {
        std::vector<std::string> result;
        const std::string folder = presetsDirectory(dataDirectory);
        wxDir dir(wxString::FromUTF8(folder.c_str()));
        if (!dir.IsOpened()) {
                return result;
        }

        wxString filename;
        bool cont = dir.GetFirst(&filename, "*.json", wxDIR_FILES);
        while (cont) {
                wxFileName full(wxString::FromUTF8(folder.c_str()));
                full.SetFullName(filename);
                result.push_back(full.GetName().ToStdString());
                cont = dir.GetNext(&filename);
        }

        std::sort(result.begin(), result.end());
        return result;
}

BiomePreset LoadPreset(const std::string& dataDirectory, const std::string& name) {
        const std::string folder = presetsDirectory(dataDirectory);
        wxFileName path(wxString::FromUTF8(folder.c_str()));
        path.SetFullName(wxString::FromUTF8((name + ".json").c_str()));

        std::ifstream in(path.GetFullPath().ToStdString(), std::ios::binary);
        if (!in.is_open()) {
                throw std::runtime_error("Could not open biome preset: " + path.GetFullPath().ToStdString());
        }

        json_spirit::Value root;
        if (!json_spirit::read(in, root) || root.type() != json_spirit::obj_type) {
                throw std::runtime_error("Malformed biome preset file: " + path.GetFullPath().ToStdString());
        }

        const json_spirit::Object& obj = root.get_obj();
        BiomePreset preset;

        json_spirit::Value seaVal = json_spirit::find_value(obj, "sea_level");
        if (seaVal.type() == json_spirit::real_type) {
                preset.seaLevel = seaVal.get_real();
        } else if (seaVal.type() == json_spirit::int_type) {
                preset.seaLevel = static_cast<double>(seaVal.get_int());
        }
        json_spirit::Value mountainVal = json_spirit::find_value(obj, "mountain_level");
        if (mountainVal.type() == json_spirit::real_type) {
                preset.mountainLevel = mountainVal.get_real();
        } else if (mountainVal.type() == json_spirit::int_type) {
                preset.mountainLevel = static_cast<double>(mountainVal.get_int());
        }

        json_spirit::Value tilesVal = json_spirit::find_value(obj, "tiles");
        if (tilesVal.type() != json_spirit::obj_type) {
                throw std::runtime_error("Biome preset missing tiles table: " + path.GetFullPath().ToStdString());
        }
        const json_spirit::Object& tilesObj = tilesVal.get_obj();
        preset.tiles.deepWater = readTileId(tilesObj, "deep_water");
        preset.tiles.shallowWater = readTileId(tilesObj, "shallow_water");
        preset.tiles.sand = readTileId(tilesObj, "sand");
        preset.tiles.grass = readTileId(tilesObj, "grass");
        preset.tiles.rock = readTileId(tilesObj, "rock");
        preset.tiles.snow = readTileId(tilesObj, "snow");
        preset.tiles.dirt = readTileId(tilesObj, "dirt");

        json_spirit::Value decorVal = json_spirit::find_value(obj, "decor");
        if (decorVal.type() == json_spirit::array_type) {
                for (const json_spirit::Value& entryVal : decorVal.get_array()) {
                        if (entryVal.type() != json_spirit::obj_type) {
                                continue;
                        }
                        const json_spirit::Object& entryObj = entryVal.get_obj();
                        DecorEntry entry;
                        json_spirit::Value nameVal = json_spirit::find_value(entryObj, "name");
                        if (nameVal.type() == json_spirit::str_type) {
                                entry.name = nameVal.get_str();
                        }
                        json_spirit::Value densityVal = json_spirit::find_value(entryObj, "density");
                        if (densityVal.type() == json_spirit::real_type) {
                                entry.density = densityVal.get_real();
                        } else if (densityVal.type() == json_spirit::int_type) {
                                entry.density = static_cast<double>(densityVal.get_int());
                        }
                        entry.tileId = readTileId(entryObj, "tile");
                        preset.decor.push_back(entry);
                }
        }

        json_spirit::Value spawnVal = json_spirit::find_value(obj, "spawn");
        if (spawnVal.type() == json_spirit::obj_type) {
                const json_spirit::Object& spawnObj = spawnVal.get_obj();
                preset.spawn.templeTile = readTileId(spawnObj, "temple_tile");
                json_spirit::Value radiusVal = json_spirit::find_value(spawnObj, "radius");
                if (radiusVal.type() == json_spirit::int_type) {
                        preset.spawn.radius = radiusVal.get_int();
                }
                preset.spawn.enabled = true;
        }

        return preset;
}

void ApplyTerrainChunk(
        Map& map,
        const BiomePreset& preset,
        const GenOptions& options,
        const std::vector<double>& height,
        const std::vector<double>& temperature,
        const std::vector<double>& moisture,
        int startX,
        int startY,
        int endX,
        int endY,
        std::mt19937_64& rng
) {
        std::uniform_real_distribution<double> random01(0.0, 1.0);

        for (int y = startY; y < endY; ++y) {
                for (int x = startX; x < endX; ++x) {
                        const size_t index = static_cast<size_t>(y) * static_cast<size_t>(options.width) + static_cast<size_t>(x);
                        const double h = height[index];
                        const double t = temperature[index];
                        const double m = moisture[index];

                        Tile* tile = obtainTile(map, x, y, options.overworldZ);
                        clearTile(tile);

                        uint16_t baseTile = preset.tiles.grass;
                        if (h < preset.seaLevel * 0.85) {
                                baseTile = preset.tiles.deepWater;
                        } else if (h < preset.seaLevel) {
                                baseTile = preset.tiles.shallowWater;
                        } else if (h < preset.seaLevel + 0.02) {
                                baseTile = preset.tiles.sand;
                        } else if (h > preset.mountainLevel) {
                                baseTile = t < 0.4 ? preset.tiles.snow : preset.tiles.rock;
                        } else if (h > preset.seaLevel + 0.2 && m < 0.35) {
                                baseTile = preset.tiles.dirt;
                        }

                        tile->addItem(Item::Create(baseTile));

                        if (baseTile == preset.tiles.deepWater || baseTile == preset.tiles.shallowWater) {
                                continue;
                        }

                        for (const DecorEntry& decor : preset.decor) {
                                double chance = decor.density;
                                if (decor.name.find("tree") != std::string::npos) {
                                        chance *= (0.5 + m * 0.5);
                                } else if (decor.name.find("bush") != std::string::npos) {
                                        chance *= (0.3 + m * 0.7);
                                }

                                if (random01(rng) < chance) {
                                        tile->addItem(Item::Create(decor.tileId));
                                }
                        }
                }
        }
}

} // namespace Biomes

