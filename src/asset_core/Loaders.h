#pragma once
#include "AssetLoader.h"

// <30 seconds
class AudioClipLoader : public AssetLoader
{
public:
	Ref<Asset> LoadAsset(const AssetMetaData& meta) override;
};

// >=30 seconds
class AudioStreamLoader : public AssetLoader
{
public:
	Ref<Asset> LoadAsset(const AssetMetaData& meta) override;
};

class MaterialLoader : public AssetLoader
{
public:
	Ref<Asset> LoadAsset(const AssetMetaData& meta) override;
};


class MeshLoader : public AssetLoader
{
public:
	Ref<Asset> LoadAsset(const AssetMetaData& meta) override;
};

class ModelLoader : public AssetLoader
{
public:
	Ref<Asset> LoadAsset(const AssetMetaData& meta) override;
};

class ShaderLoader : public AssetLoader
{
public:
	Ref<Asset> LoadAsset(const AssetMetaData& meta) override;
};

class TextureLoader : public AssetLoader
{
public:
	Ref<Asset> LoadAsset(const AssetMetaData& meta) override;
};


