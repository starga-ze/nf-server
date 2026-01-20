#include "Random.h"

#include <iomanip>

Random::Random() {
    std::random_device rd;
    m_generator.seed(rd());
}

Random::Random(uint32_t fixed_seed) {
    m_generator.seed(fixed_seed);
}

double Random::getRandomReal(double min, double max) {
    std::uniform_real_distribution<> distrib(min, max);
    return distrib(m_generator);
}

