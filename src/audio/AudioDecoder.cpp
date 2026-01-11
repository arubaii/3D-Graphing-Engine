#include "AudioDecoder.h"
#include <stdexcept>


AudioDecodeResult AudioDecoder::Decode(const std::filesystem::path&)
{
	// STUB — miniaudio / stb_vorbis later
	throw std::runtime_error("AudioDecoder::Decode not implemented");
}

void* AudioDecoder::CreateStream(const std::filesystem::path&)
{
	// STUB
	return nullptr;
}

AudioStreamInfo AudioDecoder::GetStreamInfo(void*)
{
	return {};
}
