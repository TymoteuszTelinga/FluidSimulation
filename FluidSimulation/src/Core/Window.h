#pragma once

#include <GLFW/glfw3.h>

class Window
{
public:
	Window(uint32_t width, uint32_t height);
	~Window();

	bool Init();
	void Shutdown();
	void OnUpdate();
	bool IsOpen() const;

	GLFWwindow* GetWindow() const { return m_Window; }

private:

	struct WindowData
	{
		//EventCallbackFn callback;
		uint32_t width;
		uint32_t height;
	};

	WindowData m_Data;
	GLFWwindow* m_Window;
};