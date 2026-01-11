#pragma once
#include <cstdint>
#include <glm/vec3.hpp>
#include <cassert>
#include "core/Scene.h"



class Entity
{
private:
	entt::entity m_EntityHandle{ entt::null }; // Strong typing
	Scene* m_Scene = nullptr;

public:
	Entity() = default;
	Entity(entt::entity handle, Scene* scene)
	: m_EntityHandle(handle), m_Scene(scene) { }
	Entity(const Entity& other) = default;

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		// assert(HasComponent<T>() && "entity has component already");
		T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		m_Scene->OnComponentAdded<T>(*this, component);
		return component;
	}

	/**
	 * @brief Retrieves a reference to the entity's component of type T.
	 *
	 * @note Asserts if the entity does not have the requested component.
	 */
	template<typename T>
	T& GetComponent()
	{
		assert(HasComponent<T>() && "entity does not have component");
		return m_Scene->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	const T& GetComponent() const
	{
		assert(HasComponent<T>() && "Entity does not have component");
		return m_Scene->m_Registry.get<T>(m_EntityHandle);
	}

	template<typename T>
	bool HasComponent() const
	{
		return m_Scene->m_Registry.any_of<T>(m_EntityHandle);
	}

	template<typename T>
	void RemoveComponent()
	{
		assert(HasComponent<T>() && "entity does not have component");
		m_Scene->m_Registry.remove<T>(m_EntityHandle);
	}

	operator bool()			const { return m_EntityHandle != entt::null; }
	operator entt::entity() const { return m_EntityHandle; }
	operator uint32_t()		const { return (uint32_t)m_EntityHandle; }

	bool operator!=(const Entity& other) const { return !(*this == other); }
	bool operator==(const Entity& other) const
	{ return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene; }

	const std::string& GetName() { return GetComponent<TagComponent>().Tag;}
	UUID GetUUID()				 { return GetComponent<IDComponent>().ID;}

	glm::vec3& GetPosition()
	{
		return GetComponent<TransformComponent>().Translation;
	}

	const glm::vec3& GetPosition() const
	{
		return GetComponent<TransformComponent>().Translation;
	}
};





