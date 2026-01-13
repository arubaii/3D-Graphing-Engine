#pragma once
#include <glm/vec3.hpp>


struct RayHit
{
	bool Hit = false;
	float t = std::numeric_limits<float>::max(); // distance along ray
	glm::vec3 position{};
	glm::vec3 normal{};
	entt::entity entity = entt::null;
};