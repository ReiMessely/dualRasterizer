#include "pch.h"
#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

#include "HelperFuncts.h"

dae::Texture::Texture(ID3D11Device* pDevice, const std::string& filePath)
{
	m_pSurface = IMG_Load(filePath.c_str());

	// Texture description
	const DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = m_pSurface->w;
	desc.Height = m_pSurface->h;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	// InitData SDL_Surface
	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = m_pSurface->pixels;
	initData.SysMemPitch = static_cast<UINT>(m_pSurface->pitch);
	initData.SysMemSlicePitch = static_cast<UINT>(m_pSurface->h * m_pSurface->pitch);

	HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pResource);

	// ShaderResourceView description
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc{};
	SRVDesc.Format = format;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	hr = pDevice->CreateShaderResourceView(m_pResource, &SRVDesc, &m_pShaderResourceView);
}

dae::Texture::~Texture()
{
	if (m_pSurface)
	{
		SAFE_RELEASE(m_pResource);
		SAFE_RELEASE(m_pShaderResourceView);

		SDL_FreeSurface(m_pSurface);
		m_pSurface = nullptr;
	}
}

dae::ColorRGB dae::Texture::Sample(const Vector2& uv) const
{
	Uint8 r, g, b;

	const size_t x{ static_cast<size_t>(uv.x * m_pSurface->w) };
	const size_t y{ static_cast<size_t>(uv.y * m_pSurface->h) };

	const Uint32 pixel{ m_pSurfacePixels[x + y * m_pSurface->w] };

	SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

	const constexpr float invClampVal{ 1 / 255.f };

	return { r * invClampVal,g * invClampVal,b * invClampVal };
}

ID3D11Texture2D* dae::Texture::GetResource() const
{
	return m_pResource;
}

ID3D11ShaderResourceView* dae::Texture::GetShaderResourceView() const
{
	return m_pShaderResourceView;
}
