#pragma once
#include <SDL_surface.h>
#include <string>
#include "ColorRGB.h"

namespace dae
{
	struct Vector2;

	class Texture final
	{
	public:
		Texture(ID3D11Device* pDevice, const std::string& filePath);
		~Texture();

		ColorRGB Sample(const Vector2& uv) const;

		ID3D11Texture2D* GetResource() const;
		ID3D11ShaderResourceView* GetShaderResourceView() const;

	private:
		SDL_Surface* m_pSurface{ nullptr };
		uint32_t* m_pSurfacePixels{ nullptr };

		ID3D11Texture2D* m_pResource{};
		ID3D11ShaderResourceView* m_pShaderResourceView{};
	};
}