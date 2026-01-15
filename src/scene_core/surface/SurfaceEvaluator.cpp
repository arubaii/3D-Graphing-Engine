#include "SurfaceEvaluator.h"


SurfaceEvaluator::SurfaceEvaluator(
	SurfaceType type,
	MathParser::CompiledExpression f,
	std::optional<MathParser::CompiledExpression> g
)
	: m_SurfaceType(type),
	  m_f(std::move(f)),
	  m_g(std::move(g))
{
	switch (m_SurfaceType)
	{
		case SurfaceType::ExplicitX:
			// z = f(x)
			if (m_g.has_value())
				throw std::runtime_error(
					"ExplicitX surface does not accept a secondary function"
				);
			break;

		case SurfaceType::ExplicitXY:
			// z = f(x,y)
			if (m_g.has_value())
				throw std::runtime_error(
					"ExplicitXY surface does not accept a secondary function"
				);
			break;

		case SurfaceType::CompositionX:
			// z = f(x, g(x))
			if (!m_g.has_value())
				throw std::runtime_error(
					"CompositionX surface requires secondary function g(x)"
				);
			break;

		case SurfaceType::CompositionY:
			// z = f(g(y), y)
			if (!m_g.has_value())
				throw std::runtime_error(
					"CompositionY surface requires secondary function g(y)"
				);
			break;
	}
}


SurfaceSample SurfaceEvaluator::Eval(float x, float y) const
{
	try
	{
		double z;
		switch (m_SurfaceType)
		{
			case SurfaceType::ExplicitX:
			{
				m_f.set_vars( {{"x",x}} );
				z = m_f.value();
				break;
			}

			case SurfaceType::ExplicitXY:
			{
				m_f.set_vars({{"x", x}, {"y", y}});
				z = m_f.value();
				break;
			}

			case SurfaceType::CompositionX:
			{
				m_g->set_vars({{"x", x}});
				double gx = m_g->value();
				m_f.set_vars({{"x", x}, {"y", gx}});
				z = m_f.value();
				break;
			}

			case SurfaceType::CompositionY:
			{
				m_g->set_vars({{"y", y}});
				double gy = m_g->value();
				m_f.set_vars({{"x", gy}, {"y", y}});
				z = m_f.value();
				break;
			}
		}
		if (!std::isfinite(z)) // Undefined points become holes
			return {0.0f, false};

		return {static_cast<float>(z), true};
	}
	catch (...)
	{
		return {0.0, false};
	}
}