#pragma once
#include "AssetMetaData.h"
#include "utils/SmartPtrs.h"


class AssetLoader
{
public:
	virtual ~AssetLoader() = default;
	virtual Ref<Asset> LoadAsset(const AssetMetaData&) = 0;
};