#include "Precompiled.h"
#include "ResourceManager.h"

#include "Texture.h"

ResourceManager::ResourceManager()
{
}
ResourceManager::~ResourceManager()
{
}

Texture* ResourceManager::LoadTextureFromFile(const std::wstring& fileName)
{
	if (!m_TextureCacheMap.count(fileName))
	{
		auto texture = Texture::Create(fileName);

		if (texture == nullptr)
			return nullptr;

		m_TextureCacheMap[fileName] = texture;
	}

	return m_TextureCacheMap[fileName];
}

Sound* ResourceManager::LoadSoundFromFile(const std::wstring& fileName)
{
	return nullptr;
}
