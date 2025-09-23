#pragma once

#include <glm/glm.hpp>

namespace Random
{
	uint32_t rand(const uint32_t _min, const uint32_t _max);
	float randf();
	float randf(const float min, const float max);
	glm::vec3 rand3f(const glm::vec3& min, const glm::vec3& max);
}