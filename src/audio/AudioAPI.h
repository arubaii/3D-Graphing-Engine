#pragma once
#include <cstdint>


class AudioAPI
{
public:
	static uint32_t CreateBuffer(
		const float* samples,
		uint32_t sampleCount,
		uint32_t channels,
		uint32_t sampleRate
	);
};
