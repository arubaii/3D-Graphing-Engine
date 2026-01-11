#pragma once
#include <yaml-cpp/yaml.h>
#include "utils/UUID.h"

namespace YAML
{
	template<>
	struct convert<UUID>
	{
		static Node encode(const UUID& rhs)
		{
			Node node;
			node = static_cast<uint64_t>(rhs);
			return node;
		}

		static bool decode(const Node& node, UUID& rhs)
		{
			if (!node.IsScalar())
				return false;

			rhs = UUID(node.as<uint64_t>());
			return true;
		}
	};
}