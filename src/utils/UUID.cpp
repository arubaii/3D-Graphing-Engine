#include "UUID.h"
#include <random>

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<uint64_t> uniform;

UUID::UUID()			  : m_UUID(uniform(gen)) {}
UUID::UUID(uint64_t uuid) : m_UUID(uuid)	     {}
