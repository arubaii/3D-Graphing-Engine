#include "core/Application.h"


int main()
{
	ApplicationProperties appProps;
	appProps.Name = "App Name";
	appProps.WindowProps.Title = "3D Graphing Engine";
	appProps.WindowProps.MonitorSelected = 0; // 0 is the main monitor

	Application app(appProps);
	app.Run();
}

