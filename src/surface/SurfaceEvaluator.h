#pragma once
#include <glm/vec3.hpp>

#include "math_parser.h"
#include "SurfaceTypes.h"
#include "utils/SmartPtrs.h"


struct SurfaceSample
{
	float z;
	bool valid;
};

class SurfaceEvaluator
{
private:
	Ref<MathParser::CompiledExpression> m_f;
	Ref<MathParser::CompiledExpression> m_g;
	bool m_IsEmpty = false;

public:
	SurfaceType SurfType;
	mutable bool  m_DomainCached = false;
	mutable float m_CachedRadius = 0.0f;
	float m_MAX_DOMAIN_RANGE = 50.0f;

public:

	SurfaceEvaluator(
		SurfaceType type,
		Ref<MathParser::CompiledExpression> f,
		Ref<MathParser::CompiledExpression> g = nullptr
	);

	SurfaceSample Eval(float x, float y) const;
	float Eval3(float x, float y, float z) const;

	std::function<float(float,float,float)> GetCallableImplicit() const;

	float EstimateImplicitDomainRadius() const;

	bool IsImplicitLikelyInfinite() const;


	bool IsEmpty() const { return m_IsEmpty; }

	float GetMaxDomainRange() const { return m_MAX_DOMAIN_RANGE; }

};