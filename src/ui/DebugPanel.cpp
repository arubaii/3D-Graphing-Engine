#include "DebugPanel.h"
#include "math_parser.h"
#include "surface/SurfaceTypes.h"
#include "utils/Log.h"


std::vector<std::string> GetParamsFromFunctionBuffer(const char* buff)
{
	const char* open = strchr(buff, '(');
	const char* close = strchr(buff, ')');

	if (!open || !close || close <= open)
		throw std::runtime_error("Malformed function header");

	open++;

	std::vector<std::string> params;
	const char* start = open;

	while (start < close)
	{
		// Find next comma or ')'
		const char* end = strchr(start, ',');
		if (!end || end > close)
			end = close;

		// Extract substring
		std::string token(start, end - start);

		// Trim whitespace
		auto trim = [&](std::string& s) {
			size_t a = s.find_first_not_of(" \t\n\r");
			size_t b = s.find_last_not_of(" \t\n\r");
			s = (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
		};

		trim(token);
		if (!token.empty())
			params.push_back(token);

		// Move to next start
		start = end + 1;
	}

	return params;
}

static SurfaceType DeduceSurfaceType(const std::vector<std::string>& params)
{
	if (params.size() == 1)
	{
		if (params[0] != "x")
			throw std::runtime_error("For 1-parameter functions use f(x)");
		return SurfaceType::ExplicitX;
	}

	if (params.size() == 2)
	{
		bool hasX = false, hasY = false;
		for (auto& p : params) { hasX |= (p == "x"); hasY |= (p == "y"); }
		if (!hasX || !hasY)
			throw std::runtime_error("For 2-parameter surfaces use f(x,y)");
		return SurfaceType::ExplicitXY;
	}

	if (params.size() == 3)
	{
		bool hasX = false, hasY = false, hasZ = false;
		for (auto& p : params) { hasX |= (p == "x"); hasY |= (p == "y"); hasZ |= (p == "z"); }
		if (!hasX || !hasY || !hasZ)
			throw std::runtime_error("For implicit surfaces use f(x,y,z)");
		return SurfaceType::Implicit;
	}

	throw std::runtime_error("Unsupported parameter count (use 1, 2, or 3 parameters)");
}



void DebugPanel::Render(DebugData& data)
{
    ImGui::Begin("Debug");
    ImGui::Text("FPS: %d", data.fps);
    ImGui::Text("Frame Time: %.3f ms", data.frameTime);
    ImGui::Text("Flight Mode:");
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)",
                data.cameraPos.x, data.cameraPos.y, data.cameraPos.z);
    ImGui::Text("Camera View (Pitch, Yaw): (%.2f, %.2f)",
                data.pitch, data.yaw);

	ImGui::PushItemWidth(70);
	ImGui::SliderFloat("Background Greyscale", &data.greyScale, 0.0f, 1.0f, "%.2f");
	ImGui::PopItemWidth();
	if (ImGui::Button("Show Grid"))
		data.showGrid = !data.showGrid;
	if (ImGui::Button("Show Bounding Box"))
		data.showBox = !data.showBox;



	// ====================================== PARSING ========================================


	static char functionBuffer[12] = "";
	static char expressionBuffer[512] = "";
	static char radiusBuffer[12] = "";




	struct PresetEq
	{
		const char* label;
		const char* func;
		const char* expr;
	};

static const PresetEq kPresets[] = {
	{ "##", "", "" },
	// -------------------- EXPLICIT --------------------
	{ "cos(x)+sin(y)",              "f(x,y)",   "cos(x) + sin(y)" },
	{ "Paraboloid",                 "f(x,y)",   "x^2 + y^2" },
	{ "Saddle",                     "f(x,y)",   "x^2 - y^2" },
	{ "Ripple",                     "f(x,y)",   "sin(x) + cos(y)" },
	{ "Radial ripple",              "f(x,y)",   "sin(sqrt(x*x + y*y))" },

	// -------------------- IMPLICIT  --------------------
	{ "Sphere r=3",                 "f(x,y,z)", "x^2 + y^2 + z^2 - 9" },
	{ "Sphere r=6",                 "f(x,y,z)", "x^2 + y^2 + z^2 - 36" },

	{ "Ellipsoid (3,2,1)",          "f(x,y,z)", "x^2/9 + y^2/4 + z^2 - 1" },
	{ "Cylinder (y-axis) r=3",      "f(x,y,z)", "x^2 + z^2 - 9" },
	{ "Capsule-ish (two spheres)",  "f(x,y,z)", "(x*x + y*y + (z-3)*(z-3) - 4) * (x*x + y*y + (z+3)*(z+3) - 4)" },

	{ "Torus (R=8,r=2)",            "f(x,y,z)", "(sqrt(x*x + z*z) - 8)^2 + y*y - 4" },
	{ "Thin torus (R=8,r=1)",       "f(x,y,z)", "(sqrt(x*x + z*z) - 8)^2 + y*y - 1" },
	{ "Fat torus (R=6,r=3)",        "f(x,y,z)", "(sqrt(x*x + z*z) - 6)^2 + y*y - 9" },

	{"Genus 3",						"f(x,y,z)", "(x^2-1)^2+(y^2-1)^2+(z^2-1)^2+4(x^2y^2+x^2z^2+y^2z^2)+8x*y*z-2(x^2+y^2+z^2)"},
	{ "Spiky Thing",				"f(x,y,z)", "z^6-5(x^2+y^2)z^4+5(x^2+y^2)^2z^2+2(5x^4-10x^2y^2+y^4)y*z-1.002(x^2+y^2+z^2)^3+0.2" },


	{ "Cone (double)",              "f(x,y,z)", "x^2 + z^2 - y^2" },
	{ "Hyperboloid (1-sheet)",      "f(x,y,z)", "x^2 + z^2 - y^2 - 1" },
	{ "Hyperboloid (2-sheet)",      "f(x,y,z)", "y^2 - x^2 - z^2 - 1" },


	{ "Tangent cylinder",           "f(x,y,z)", "x^2 + y^2 - 1" },
	{ "Heart",                      "f(x,y,z)", "x^2+4y^2+(1.15z-0.6(2(x^2+.05y^2+.001)^0.7+y^2)^0.3+0.3)^2-1" },


	{ "Klein bottle (Bottom)", "f(x,y,z)", "(x^2 + y^2 + z^2 - 1)^2 - 4(x^2 + y^2)" },
	{ "Quartic blob",               "f(x,y,z)", "x^4 + y^4 + z^4 - 25" },

};

	static int presetIdx = 0;

	ImGui::PushItemWidth(260);

	if (ImGui::BeginCombo("Load Preset Function", kPresets[presetIdx].label))
	{
		for (int i = 0; i < (int)(sizeof(kPresets) / sizeof(kPresets[0])); ++i)
		{
			const bool selected = (presetIdx == i);
			if (ImGui::Selectable(kPresets[i].label, selected))
				presetIdx = i;
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();



	static int  lastPresetIdx   = -1;
	static bool requestAutoSet  = true;

	auto ApplyEquationFromBuffers = [&]()
	{
		try
		{
			std::string s(expressionBuffer);
			data.expression->set_expression(s);

			std::set<std::string> vars = data.expression->get_vars();
			std::vector<std::string> params = GetParamsFromFunctionBuffer(functionBuffer);

			std::unordered_set<std::string> allowed(params.begin(), params.end());

			for (const auto& v : vars)
			{
				if (!allowed.contains(v))
					throw std::runtime_error("Function parameters do not match expression variables");
			}

			data.surfaceType = DeduceSurfaceType(params);
			data.expressionDirty = true;
		}
		catch (const std::exception& e)
		{
			LOG_ERROR(std::string("[Math Parser] Auto Set failed: ") + e.what());
		}
	};

	if (requestAutoSet)
	{
		presetIdx = 0;

		functionBuffer[0]   = '\0';
		expressionBuffer[0] = '\0';


		requestAutoSet = false;
		lastPresetIdx  = presetIdx;
	}



	if (presetIdx != lastPresetIdx)
	{
		std::snprintf(functionBuffer,   IM_ARRAYSIZE(functionBuffer),   "%s", kPresets[presetIdx].func);
		std::snprintf(expressionBuffer, IM_ARRAYSIZE(expressionBuffer), "%s", kPresets[presetIdx].expr);

		ApplyEquationFromBuffers();
		lastPresetIdx = presetIdx;
	}


	using namespace MathParser;
	ImGui::PushItemWidth(70);
	ImGui::InputTextWithHint("##Function", "f(x,...)", functionBuffer, IM_ARRAYSIZE(functionBuffer));
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("=");
	ImGui::SameLine();
	ImGui::PushItemWidth(200);
	ImGui::InputTextWithHint(
		"##Expression",
		"expression",
		expressionBuffer,
		IM_ARRAYSIZE(expressionBuffer),
		ImGuiInputTextFlags_EnterReturnsTrue);

	ImGui::PopItemWidth();


	ImGui::SameLine();



	if (ImGui::Button("Set Equation"))
	{
		try
		{
			std::string s(expressionBuffer);
			data.expression->set_expression(s);

			std::set<std::string> vars = data.expression->get_vars();
			std::vector<std::string> params = GetParamsFromFunctionBuffer(functionBuffer);

			std::unordered_set<std::string> allowed(params.begin(), params.end());

			for (const auto& v : vars)
			{
				if (!allowed.contains(v))
					throw std::runtime_error("Function parameters do not match expression variables");
			}

			data.surfaceType = DeduceSurfaceType(params);
			data.expressionDirty = true;
		}
		catch (const std::exception& e)
		{
			LOG_ERROR(std::string("[Math Parser] Set Equation failed: ") + e.what());
		}
	}




    ImGui::End();
}

