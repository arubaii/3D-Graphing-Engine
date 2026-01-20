#include "SurfaceEvaluator.h"
#include "utils/Log.h"
#include "utils/SmartPtrs.h"
#include <cmath>


SurfaceEvaluator::SurfaceEvaluator(
	SurfaceType type,
	Ref<MathParser::CompiledExpression> f,
	Ref<MathParser::CompiledExpression> g
)
	: SurfType(type),
	  m_f(std::move(f)),
	  m_g(std::move(g))
{
	if (!m_f || m_f->get_length() == 0)
		m_IsEmpty = true;

	switch (SurfType)
	{
		case SurfaceType::ExplicitX:
		case SurfaceType::ExplicitXY:
			if (m_g) throw std::runtime_error("This surface type does not accept g()");
			break;

		case SurfaceType::CompositionX:
		case SurfaceType::CompositionY:
			if (!m_g) throw std::runtime_error("This surface type requires g()");
			break;

		case SurfaceType::Implicit:
			break;
	}
}

SurfaceSample SurfaceEvaluator::Eval(float x, float y) const
{
	try
	{
		double z = 0.0;

		switch (SurfType)
		{
			case SurfaceType::ExplicitX:
			{
				m_f->set_vars({{"x", x}});
				z = m_f->value();
				break;
			}

			case SurfaceType::ExplicitXY:
			{
				m_f->set_vars({{"x", x}, {"y", y}});
				z = m_f->value();
				break;
			}

			case SurfaceType::CompositionX:
			{
				m_g->set_vars({{"x", x}});
				double gx = m_g->value();

				m_f->set_vars({{"x", x}, {"y", gx}});
				z = m_f->value();
				break;
			}

			case SurfaceType::CompositionY:
			{
				m_g->set_vars({{"y", y}});
				double gy = m_g->value();

				m_f->set_vars({{"x", gy}, {"y", y}});
				z = m_f->value();
				break;
			}

			case SurfaceType::Implicit:
				return {0.0f, false};
		}

		if (!std::isfinite(z))
			return {0.0f, false};

		return {static_cast<float>(z), true};
	}
	catch (...)
	{
		return {0.0f, false};
	}
}

float SurfaceEvaluator::Eval3(float x, float y, float z) const
{
	if (!m_f)
		return 0.0f;

	try
	{
		m_f->set_vars("x", x);
		m_f->set_vars("y", y);
		m_f->set_vars("z", z);

		double val = m_f->value();

		if (!std::isfinite(val))
			return 0.0f;

		return float(val);
	}
	catch (...)
	{
		return 0.0f;
	}

}

std::function<float(float,float,float)>
SurfaceEvaluator::GetCallableImplicit() const
{
	auto expr = m_f;
	if (!expr)
		return [](float, float, float) { return 0.0f; };

	// Ensure variables exist in the var table before grabbing pointers
	expr->set_vars("x", 0.0);
	expr->set_vars("y", 0.0);
	expr->set_vars("z", 0.0);

	double* px = expr->GetVarPtr("x");
	double* py = expr->GetVarPtr("y");
	double* pz = expr->GetVarPtr("z");

	if (!px || !py || !pz)
	{

		return [expr](float x, float y, float z) -> float
		{
			expr->set_vars("x", (double)x);
			expr->set_vars("y", (double)y);
			expr->set_vars("z", (double)z);
			return (float)expr->value();
		};
	}

	return [expr, px, py, pz](float x, float y, float z) -> float
	{
		*px = (double)x;
		*py = (double)y;
		*pz = (double)z;
		return (float)expr->value();
	};
}

static float ProbeImplicitBoundary_LastCross(
	const SurfaceEvaluator& eval,
	const glm::vec3& dir,
	float maxRange,
	float step)
{
	glm::vec3 d = glm::normalize(dir);

	auto F = [&](float t) -> float {
		glm::vec3 p = d * t;
		return eval.Eval3(p.x, p.y, p.z); // MUST be scalar field
	};

	float t0 = 0.0f;
	float f0 = F(t0);

	bool found = false;
	float lastA = 0.0f, lastB = 0.0f;
	float fa = 0.0f, fb = 0.0f;

	for (float t1 = step; t1 <= maxRange; t1 += step)
	{
		float f1 = F(t1);

		// sign change (or exact hit)
		if ((f0 <= 0.0f && f1 >= 0.0f) || (f0 >= 0.0f && f1 <= 0.0f))
		{
			found = true;
			lastA = t0; lastB = t1;
			fa = f0; fb = f1;
		}

		t0 = t1;
		f0 = f1;
	}

	if (!found)
		return 0.0f; // caller will fallback / clamp

	// Bisection on last [A,B]
	float lo = lastA, hi = lastB;
	float flo = fa, fhi = fb;

	for (int i = 0; i < 16; ++i)
	{
		float mid = 0.5f * (lo + hi);
		float fmid = F(mid);

		if ((flo <= 0.0f && fmid >= 0.0f) || (flo >= 0.0f && fmid <= 0.0f))
		{
			hi = mid;
			fhi = fmid;
		}
		else
		{
			lo = mid;
			flo = fmid;
		}
	}

	return 0.5f * (lo + hi);
}

float SurfaceEvaluator::EstimateImplicitDomainRadius() const
{
	if (!m_f)
		return 2.0f;

	auto F = [&](float x, float y, float z) -> float
	{
		m_f->set_vars("x", x);
		m_f->set_vars("y", y);
		m_f->set_vars("z", z);
		double v = m_f->value();
		if (!std::isfinite(v)) return 1e30f;
		return (float)v; // field value (iso assumed 0)
	};


	constexpr float STEP        = 0.25f;
	constexpr float PADDING     = 1.0f;

	static const glm::vec3 dirs[] =
	{
		{ 1, 0, 0 }, { -1, 0, 0 },
		{ 0, 1, 0 }, {  0,-1, 0 },
		{ 0, 0, 1 }, {  0, 0,-1 },

		{ 1, 1, 0 }, { -1, 1, 0 }, { 1,-1, 0 }, { -1,-1, 0 },
		{ 1, 0, 1 }, { -1, 0, 1 }, { 1, 0,-1 }, { -1, 0,-1 },
		{ 0, 1, 1 }, {  0,-1, 1 }, { 0, 1,-1 }, {  0,-1,-1 },

		{ 1, 1, 1 }, { -1, 1, 1 }, { 1,-1, 1 }, { 1, 1,-1 },
		{ -1,-1, 1 },{ -1, 1,-1 },{ 1,-1,-1 },{ -1,-1,-1 }
	};

	auto ProbeLastCross = [&](const glm::vec3& dir) -> float
	{
		glm::vec3 d = glm::normalize(dir);

		float t0 = 0.0f;
		float f0 = F(0.0f, 0.0f, 0.0f);

		bool found = false;
		float lastA = 0.0f, lastB = 0.0f;
		float fa = f0, fb = f0;

		for (float t1 = STEP; t1 <= m_MAX_DOMAIN_RANGE; t1 += STEP)
		{
			glm::vec3 p1 = d * t1;
			float f1 = F(p1.x, p1.y, p1.z);

			if ((f0 <= 0.0f && f1 >= 0.0f) || (f0 >= 0.0f && f1 <= 0.0f))
			{
				found = true;
				lastA = t0; lastB = t1;
				fa = f0; fb = f1;
			}

			t0 = t1;
			f0 = f1;
		}

		if (!found)
			return 0.0f;

		float lo = lastA, hi = lastB;
		float flo = fa, fhi = fb;

		for (int i = 0; i < 18; ++i)
		{
			float mid = 0.5f * (lo + hi);
			glm::vec3 pm = d * mid;
			float fmid = F(pm.x, pm.y, pm.z);

			if ((flo <= 0.0f && fmid >= 0.0f) || (flo >= 0.0f && fmid <= 0.0f))
			{
				hi = mid; fhi = fmid;
			}
			else
			{
				lo = mid; flo = fmid;
			}
		}

		return 0.5f * (lo + hi);
	};

	float maxR = 0.0f;

	for (const glm::vec3& d : dirs)
	{
		float r = ProbeLastCross(d);
		maxR = std::max(maxR, r);
	}

	float outR = maxR + PADDING;

	if (outR <= STEP)
	{
		outR = 8.0f;
	}

	outR = std::clamp(outR, 1.0f, m_MAX_DOMAIN_RANGE);

	return outR;
}

