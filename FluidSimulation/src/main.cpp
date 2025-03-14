#include "Simulation.h"

int main()
{
	ApplicationSpecification spec;
	Simulation* app = new Simulation(spec);

	app->Run();

	delete app;

	return 0;
}