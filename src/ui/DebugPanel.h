#pragma once
#include <imgui.h>
#include <glm/glm.hpp>

struct DebugData
{
    int fps;
    float frameTime;
    bool flightMode;
    glm::vec3 cameraPos;
    float pitch, yaw;
    float greyScale = 0.1f;
};


class DebugPanel
{
public:
    static void Render(DebugData& data);
};

