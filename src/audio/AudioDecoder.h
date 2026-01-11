#pragma once
#include <filesystem>
#include <cstdint>


struct AudioDecodeResult
{
	const float* Samples = nullptr;
	uint32_t SampleCount = 0;
	uint32_t Channels = 0;
	uint32_t SampleRate = 0;
	float Duration = 0.0f;
};

struct AudioStreamInfo
{
	uint32_t Channels = 0;
	uint32_t SampleRate = 0;
	float Duration = 0.0f;
};

class AudioDecoder
{
public:
	static AudioDecodeResult Decode(const std::filesystem::path& path);
	static void* CreateStream(const std::filesystem::path& path);
	static AudioStreamInfo GetStreamInfo(void* streamHandle);
};
