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

SurfaceSample SurfaceEvaluator::Eval3(float x, float y, float z) const
{
	if (!m_f)
		return {0.0f, false};

	try
	{
		m_f->set_vars("x", x);
		m_f->set_vars("y", y);
		m_f->set_vars("z", z);

		double val = m_f->value();
		if (!std::isfinite(val))
			return {0.0f, false};

		return SurfaceSample{
			.z     = float(val),
			.valid = true
		};
	}
	catch (...)
	{
		return {0.0f, false};
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