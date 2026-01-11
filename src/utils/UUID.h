#pragma once
#include <functional>

class UUID
{
private:
	uint64_t m_UUID;
public:
	UUID();                        // generates a new UUID
	explicit UUID(uint64_t value); // construct from known value

	operator uint64_t() const { return m_UUID; }

};

namespace std
{
	template<>
	struct hash<UUID>
	{
		std::size_t operator()(const UUID& uuid) const noexcept
		{
			return std::hash<uint64_t>{}(static_cast<uint64_t>(uuid));
		}
	};
}