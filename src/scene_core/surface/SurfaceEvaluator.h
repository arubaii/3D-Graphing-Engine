#pragma once
#include "math_parser.h"

enum class SurfaceType
{
	ExplicitX,        // z = f(x)
	ExplicitXY,       // z = f(x,y)
	CompositionX,     // z = f(x, g(x))
	CompositionY      // z = f(g(y), y)
};

struct SurfaceSample
{
	float z;
	bool valid;
};

class SurfaceEvaluator
{
private:
	SurfaceType m_SurfaceType;
	mutable MathParser::CompiledExpression m_f;
	mutable std::optional<MathParser::CompiledExpression> m_g;

public:

	SurfaceEvaluator
	(
		SurfaceType sf,
		MathParser::CompiledExpression f,
		std::optional<MathParser::CompiledExpression> g = std::nullopt
	);

	SurfaceSample Eval(float x, float y) const;

};