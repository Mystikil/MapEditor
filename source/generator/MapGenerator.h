#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

class Map;

struct GenOptions {
        uint64_t seed = 1337;
        int width = 1024;
        int height = 1024;
        int overworldZ = 7;
        int caveZ = 8;
        std::string biomePreset = "temperate";
        bool makeRivers = true;
        bool makeCaves = true;
        bool makeDungeons = true;
        bool placeStarterTown = true;
        int chunkSize = 256;
};

class MapGenerator {
public:
        static std::unique_ptr<Map> Generate(const GenOptions& opt,
                                             std::function<bool(int, int)> progress = nullptr);
};

