#pragma once

#include <glm/glm.hpp>
#include <string>

namespace GUIControls
{
	bool DragVec3(const std::string& label, glm::vec3& values, float resetValue = 0.f, float columnWidth = 100.f);
}