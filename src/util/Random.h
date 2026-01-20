#pragma once

#include <random>
#include <cstdint>

class Random {
public:
    Random();

    explicit Random(uint32_t seed);

    double getRandomReal(double min, double max);

private:
    std::mt19937 m_generator;
};
