#pragma once
#include <imgui.h>
#include <glm/glm.hpp>
#include "math_parser.h"
#include "surface/SurfaceTypes.h"
#include "utils/SmartPtrs.h"


struct DebugData
{
    int fps;
    float frameTime;
    bool flightMode;
    glm::vec3 cameraPos;
    float pitch, yaw;
    float greyScale = 0.0f;
    bool showGrid = true;
    bool showBox = false;

    Ref<MathParser::CompiledExpression> expression;
    bool expressionDirty = false;

    float radius = 5.0f;

    SurfaceType surfaceType;
};


class DebugPanel
{
public:
    static void Render(DebugData& data);
};

