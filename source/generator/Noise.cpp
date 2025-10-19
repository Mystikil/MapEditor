#include "Noise.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

Noise::Noise(uint64_t seed) {
        std::vector<int> p(256);
        std::iota(p.begin(), p.end(), 0);

        std::mt19937_64 rng(seed);
        std::shuffle(p.begin(), p.end(), rng);

        for (int i = 0; i < 256; ++i) {
                permutation[i] = permutation[i + 256] = p[i];
        }
}

double Noise::sample(double x, double y) const {
        return perlin(x, y);
}

double Noise::fractal(double x, double y, int octaves, double frequency, double persistence, double lacunarity) const {
        double amplitude = 1.0;
        double maxAmplitude = 0.0;
        double value = 0.0;

        for (int i = 0; i < octaves; ++i) {
                value += amplitude * perlin(x * frequency, y * frequency);
                maxAmplitude += amplitude;
                amplitude *= persistence;
                frequency *= lacunarity;
        }

        if (maxAmplitude == 0.0) {
                return 0.0;
        }

        return value / maxAmplitude;
}

double Noise::perlin(double x, double y) const {
        const int X = static_cast<int>(std::floor(x)) & 255;
        const int Y = static_cast<int>(std::floor(y)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);

        const double u = fade(x);
        const double v = fade(y);

        const int aa = permutation[X] + Y;
        const int ab = permutation[X] + Y + 1;
        const int ba = permutation[X + 1] + Y;
        const int bb = permutation[X + 1] + Y + 1;

        const double g1 = grad(permutation[aa], x, y);
        const double g2 = grad(permutation[ba], x - 1.0, y);
        const double g3 = grad(permutation[ab], x, y - 1.0);
        const double g4 = grad(permutation[bb], x - 1.0, y - 1.0);

        const double lerpX1 = lerp(g1, g2, u);
        const double lerpX2 = lerp(g3, g4, u);

        return lerp(lerpX1, lerpX2, v);
}

double Noise::fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
}

double Noise::grad(int hash, double x, double y) {
        const int h = hash & 7;
        const double u = h < 4 ? x : y;
        const double v = h < 4 ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -2.0 * v : 2.0 * v);
}

double Noise::lerp(double a, double b, double t) {
        return a + t * (b - a);
}

