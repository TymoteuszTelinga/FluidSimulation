#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include "Events/Event.h"

class Window
{
public:
	Window(uint32_t width, uint32_t height);
	~Window();

	using EventCallbackFn = std::function<void(Event&)>;

	bool Init();
	void Shutdown();
	void OnUpdate();
	bool IsOpen() const;

	void SetEventCallback(const EventCallbackFn& callback);
	GLFWwindow* GetWindow() const { return m_Window; }

private:
	struct WindowData
	{
		EventCallbackFn callback;
		uint32_t width;
		uint32_t height;
	};

	WindowData m_Data;
	GLFWwindow* m_Window;
};