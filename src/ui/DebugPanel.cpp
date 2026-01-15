#include "DebugPanel.h"

#include "math_parser.h"
#include "utils/Log.h"
#include "math_parser.h"

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
	static char functionBuffer[12] = "";
	static char expressionBuffer[256] = "";
	static char varBuffer[16] = "";
	static char valueBuffer[64];

	using namespace MathParser;
	ImGui::PushItemWidth(70);
	ImGui::InputTextWithHint("##Function", "f(x,...)", functionBuffer, IM_ARRAYSIZE(functionBuffer));
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text("=");
	ImGui::SameLine();
	ImGui::PushItemWidth(200);
	if (ImGui::InputText(
		"Expression",
		expressionBuffer,
		IM_ARRAYSIZE(expressionBuffer),
		ImGuiInputTextFlags_EnterReturnsTrue))
	{
		LOG("Expression: ", expressionBuffer);
	}
	ImGui::PopItemWidth();

	ImGui::PushItemWidth(80);
	ImGui::InputTextWithHint("##Variable","x,...", varBuffer, IM_ARRAYSIZE(varBuffer));
	ImGui::PopItemWidth();

	ImGui::SameLine();

	ImGui::PushItemWidth(120);
	ImGui::InputTextWithHint("##Values","x_val,...", valueBuffer, IM_ARRAYSIZE(valueBuffer));
	ImGui::PopItemWidth();

	ImGui::SameLine();

	if (ImGui::Button("Set Variable"))
	{
		std::string s(expressionBuffer);
		auto vars   = ParseVars(varBuffer);
		auto values = ParseValues(valueBuffer);


		if (vars.size() != values.size())
		{
			LOG_ERROR("[Math Parser] Variable/value count mismatch");
		}

		else {
			Vars bindings;
			for (size_t i = 0; i < vars.size(); ++i)
				bindings.list.emplace_back(vars[i], values[i]);

			TRY_MATH({
					CompiledExpression expr(s, bindings);
					double result = expr.value();
					LOG("Result: ", result);
					});
		}
	}













    ImGui::End();
}
