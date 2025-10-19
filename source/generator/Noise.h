#pragma once

#include <cstdint>

class Noise {
public:
        explicit Noise(uint64_t seed);

        double sample(double x, double y) const;
        double fractal(double x, double y, int octaves, double frequency, double persistence, double lacunarity) const;

private:
        double perlin(double x, double y) const;

        static double fade(double t);
        static double grad(int hash, double x, double y);
        static double lerp(double a, double b, double t);

        int permutation[512];
};

