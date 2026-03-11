#pragma once
#include <imgui.h>
#include <glm/glm.hpp>
#include "math_parser.h"
#include "core/Scene.h"
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
    bool showGrid = false;
    bool showBox = true;

    Ref<MathParser::CompiledExpression> expression;
    bool expressionDirty = false;

    float radius = 10.0f;

    SurfaceType surfaceType;
    glm::vec4 surfaceColor = {0.272f, 0.438f, 0.683f, 1.0f};

    int octreeDepth = 6;

    int infiniteFinalRes = 128;

    bool usePastel  = true;
    bool useNormal  = false;
    bool userColor  = false;


};


class DebugPanel
{
public:
    static void Render(DebugData &data, Scene *scene);
};

