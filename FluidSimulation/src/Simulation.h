#pragma once

#include "Core/Application.h"

class Simulation : public Application
{
public:
	Simulation(const ApplicationSpecification& spec);
	~Simulation();

	virtual void OnRender() override;

private:

};
