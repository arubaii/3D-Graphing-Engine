#include "DebugPanel.h"

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

	ImGui::SliderFloat("Background Greyscale", &data.greyScale, 0.0f, 1.0f, "%.2f");
    ImGui::End();
}
