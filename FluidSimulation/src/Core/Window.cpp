
#include "Window.h"

#include <iostream>

Window::Window(uint32_t width, uint32_t height)
{
	m_Data.width = width;
	m_Data.height = height;
	Init();
}

Window::~Window()
{
	Shutdown();
}

bool Window::Init()
{
	//Initialize the library
	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	m_Window = glfwCreateWindow(m_Data.width, m_Data.height, "Fluid simulation", nullptr, nullptr);
	if (!m_Window)
	{
		std::cout << "Createion error\n";
		return false;
	}

	return false;
}

void Window::Shutdown()
{
	glfwTerminate();
}

void Window::OnUpdate()
{
	glfwPollEvents();
}

bool Window::IsOpen() const
{
	return !glfwWindowShouldClose(m_Window);
}
