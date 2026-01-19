#pragma once

enum class SurfaceType
{
	ExplicitX,         // z = f(x)
	ExplicitXY,        // z = f(x,y)
	CompositionX,      // z = f(x, *), extend for all y
	CompositionY,      // z = f(*, y), extend for all x
	Implicit		   // c = f(x,y,z)
};

inline const char* SurfaceTypeToString(SurfaceType t)
{
	switch (t)
	{
		case SurfaceType::ExplicitXY:   return "ExplicitXY";
		case SurfaceType::ExplicitX:    return "ExplicitX";
		case SurfaceType::CompositionX: return "CompositionX";
		case SurfaceType::CompositionY: return "CompositionY";
		case SurfaceType::Implicit:     return "Implicit";
		default:                        return "Unknown";
	}
}