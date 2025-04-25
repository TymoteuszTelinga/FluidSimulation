#pragma once

//#include <cstdint>
#include <glm/glm.hpp>

class Input
{
public:
	static bool IsKeyDown(uint32_t keycode);
	static bool IsMouseButtonDown(uint32_t button);

	static glm::vec2 GetMousePosition();
	static void SetCursorMode(uint32_t mode);
};