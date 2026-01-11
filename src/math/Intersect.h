#pragma once
#include <cmath>
#include "Ray.h"

inline bool RayIntersectsTriangle
(
	const Ray& ray,
	const glm::vec3& v0,
	const glm::vec3& v1,
	const glm::vec3& v2,
	float& outT,
	glm::vec3& outNormal
)
{
	constexpr float EPSILON = 1e-7f;

	glm::vec3 edge1 = v1 - v0;
	glm::vec3 edge2 = v2 - v0;

	glm::vec3 pvec = glm::cross(ray.Direction, edge2);
	float det = glm::dot(edge1, pvec);

	// Ray parallel to triangle
	if (std::fabs(det) < EPSILON)
		return false;

	float invDet = 1.0f / det;

	glm::vec3 tvec = ray.Origin - v0;
	float u = glm::dot(tvec, pvec) * invDet;
	if (u < 0.0f || u > 1.0f)
		return false;

	glm::vec3 qvec = glm::cross(tvec, edge1);
	float v = glm::dot(ray.Direction, qvec) * invDet;
	if (v < 0.0f || u + v > 1.0f)
		return false;

	float t = glm::dot(edge2, qvec) * invDet;
	if (t < 0.0f)
		return false; // hit behind ray origin

	outT = t;
	outNormal = glm::normalize(glm::cross(edge1, edge2));

	return true;
}

inline bool IntersectPlane(const Ray& ray, float y, glm::vec3& outPos)
{
	if (std::abs(ray.Direction.y) < 1e-6f)
		return false;

	float t = (y - ray.Origin.y) / ray.Direction.y;
	if (t < 0.0f)
		return false;

	outPos = ray.At(t);
	return true;
}