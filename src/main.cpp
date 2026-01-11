#include "core/Application.h"

/*
	Change project name to "Engine-Core", and use as template for future projects with application
	specific code
*/

int main()
{
	ApplicationProperties appProps;
	appProps.Name = "App Name";
	appProps.WindowProps.Width  = 1366;
	appProps.WindowProps.Height = 768;  // 16:9
	appProps.WindowProps.Title = "OpenGL";
	appProps.WindowProps.MonitorSelected = 1; // 0 is the main monitor

	Application app(appProps);
	app.Run();
}

/* Ownership:
		Application
		|---- Window  <--> IO (initializes window context and translates IO callbacks for scene reference)
		|---- Renderer
		|---- Scene
			    |---- Entities / Objects
			    |---- Camera

		Application
		   ↓
		 Scene ───→ Renderer
		   ↑
		 Window (queried, not owned)

		Application owns everything, scene tells the renderer what to render after querying window for input states.

	TODO =========================================================================================================
	1. Asset Management
	2. (Basic) Lighting System
	3. Serialization (save/load scene)
	4. (Basic) Physics & Collision
	5. Rudimentary GUI (refine progressively)
		- Debug panel
		- Drag-drop
			-- Primitives and Models
		- Hierarchy panel
			-- Select an entity in panel
			-- Delete / Duplicate
		- Inspector
			-- A panel that shows all components of the selected entity and lets you edit them, e.g. scale/position
		- Viewport (Rendered scene embedded in the GUI)
	6. Minimal Layer System
	7. Minimal Event System
	8. Tidy up -> Refine Abstractions -> Refactor
*/