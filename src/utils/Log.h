#pragma once
#include <iostream>
#include <ostream>
#include <sstream>
#include <array>
#include <algorithm>
#include <glm/glm.hpp>

#define TRY_MATH(expr)                                      \
    do {                                                    \
        try {                                               \
            expr;                                           \
        } catch (const std::exception& e) {                 \
            LOG_ERROR("[Math Parser] ", e.what());    \
        }                                                   \
    } while (0)


template<typename... Args>
void LOG(Args&&... args)
{
	(std::cout << ... << args) << '\n';
}

template<typename... Args>
void LOG_ERROR(Args&&... args)
{
	(std::cerr << ... << args) << '\n';
}

namespace glm // Logging overloads
{
	inline std::ostream& operator<<(std::ostream& os, const glm::vec2& v)
	{
		return os << "(" << v.x << ", " << v.y << ")";
	}

	inline std::ostream& operator<<(std::ostream& os, const glm::vec3& v)
	{
		return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
	}

	inline std::ostream& operator<<(std::ostream& os, const glm::vec4& v)
	{
		return os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
	}

	inline std::ostream& operator<<(std::ostream& out, const glm::mat3& M)
	{
		// Save stream formatting
		std::ios old_state(nullptr);
		old_state.copyfmt(out);
		const auto old_fill = out.fill(' ');

		auto to_string_like_stream = [&](float x) -> std::string {
			std::ostringstream ss;
			ss.copyfmt(out);
			ss.width(0);
			ss << x;
			return ss.str();
		};

		// Column widths (GLM is column-major: M[col][row])
		std::array<std::size_t, 3> w{};
		for (std::size_t j = 0; j < 3; ++j)
		{
			std::size_t maxw = 1;
			for (std::size_t i = 0; i < 3; ++i)
			{
				const std::string s = to_string_like_stream(M[j][i]);
				maxw = std::max(maxw, s.size());
			}
			w[j] = maxw;
		}

		auto put_centered = [&](const std::string& s, std::size_t width)
		{
			if (s.size() >= width) { out << s; return; }
			const std::size_t pad = width - s.size();
			const std::size_t left = pad / 2;
			const std::size_t right = pad - left;
			out << std::string(left, ' ') << s << std::string(right, ' ');
		};

		// Print rows
		for (std::size_t i = 0; i < 3; ++i)
		{
			out << "|";
			for (std::size_t j = 0; j < 3; ++j)
			{
				out << ' ';
				const std::string s = to_string_like_stream(M[j][i]);
				put_centered(s, w[j]);
			}
			out << " |\n";
		}

		// Restore formatting
		out.fill(old_fill);
		out.copyfmt(old_state);
		return out;
	}

	// ---------- mat4 ---------- //
	inline std::ostream& operator<<(std::ostream& out, const glm::mat4& M)
	{
		// Save stream formatting
		std::ios old_state(nullptr);
		old_state.copyfmt(out);
		const auto old_fill = out.fill(' ');

		auto to_string_like_stream = [&](float x) -> std::string {
			std::ostringstream ss;
			ss.copyfmt(out);
			ss.width(0);
			ss << x;
			return ss.str();
		};

		// Column widths (column-major)
		std::array<std::size_t, 4> w{};
		for (std::size_t j = 0; j < 4; ++j)
		{
			std::size_t maxw = 1;
			for (std::size_t i = 0; i < 4; ++i)
			{
				const std::string s = to_string_like_stream(M[j][i]);
				maxw = std::max(maxw, s.size());
			}
			w[j] = maxw;
		}

		auto put_centered = [&](const std::string& s, std::size_t width)
		{
			if (s.size() >= width) { out << s; return; }
			const std::size_t pad = width - s.size();
			const std::size_t left = pad / 2;
			const std::size_t right = pad - left;
			out << std::string(left, ' ') << s << std::string(right, ' ');
		};

		// Print rows
		for (std::size_t i = 0; i < 4; ++i)
		{
			out << "|";
			for (std::size_t j = 0; j < 4; ++j)
			{
				out << ' ';
				const std::string s = to_string_like_stream(M[j][i]);
				put_centered(s, w[j]);
			}
			out << " |\n";
		}

		// Restore formatting
		out.fill(old_fill);
		out.copyfmt(old_state);
		return out;
	}

}


