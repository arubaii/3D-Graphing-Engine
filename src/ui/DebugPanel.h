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
    float greyScale = 0.85f;
    bool showGrid = true;

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

