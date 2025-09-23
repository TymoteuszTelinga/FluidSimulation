#include "Random.h"

#include <cstdlib>
#include <stdint.h>

uint32_t Random::rand(const uint32_t _min, const uint32_t _max)
{
    return std::rand() % (_max - _min) + _min;
}

float Random::randf()
{
    return ((float)std::rand() / RAND_MAX);
}

float Random::randf(const float min, const float max)
{
    const float range = max - min;
    return randf() * range + min;
}

glm::vec3 Random::rand3f(const glm::vec3& min, const glm::vec3& max)
{
    float x = Random::randf(min.x, max.x);
    float y = Random::randf(min.y, max.y);
    float z = Random::randf(min.z, max.z);
    return glm::vec3(x,y,z);
}
