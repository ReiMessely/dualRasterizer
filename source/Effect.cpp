#include "pch.h"
#include "Effect.h"
#include "Texture.h"
#include "HelperFuncts.h"

namespace dae
{
	Effect::Effect(ID3D11Device* pDevice, const std::wstring& assetFile)
		: m_pEffect{ LoadEffect(pDevice, assetFile) }
		, m_pDevice{ pDevice }
	{
		m_pTechnique = m_pEffect->GetTechniqueByName("PointFilteringTechnique");
		if (!m_pTechnique->IsValid())
		{
			std::wcout << L"Technique not valid\n";
		}

		// ---- WORLD ----

		m_pMatWorldViewProjVariable = m_pEffect->GetVariableByName("gWorldViewProj")->AsMatrix();
		if (!m_pMatWorldViewProjVariable->IsValid())
		{
			std::wcout << L"m_pMatWorldViewProjVariable not valid!\n";
		}

		// ---- MAPS ----
		m_pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
		if (!m_pDiffuseMapVariable->IsValid())
		{
			std::wcout << L"m_pDiffuseMapVariable not valid!\n";
		}

		m_pRasterizerStateVariable = m_pEffect->GetVariableByName("gRasterizerState")->AsRasterizer();
		if (!m_pRasterizerStateVariable->IsValid())
		{
			std::wcout << L"m_pRasterizerVariable not valid!\n";
		}

		
	}

	Effect::~Effect()
	{
		SAFE_RELEASE(m_pRasterizerState);
		SAFE_RELEASE(m_pEffect);
	}

	ID3DX11Effect* Effect::GetEffect() const
	{
		return m_pEffect;
	}

	ID3DX11EffectTechnique* Effect::GetTechnique() const
	{
		return m_pTechnique;
	}

	void Effect::SetWorldViewProjectionMatrix(const Matrix& matrix)
	{
		// I know it looks cursed but trust me bro it works
		/*
		The last thing you need to do is update the data every frame using the SetMatrix(...) function of the (c++ side)
		matrix effect variable, same as you update the vertex buffer etc. Using your camera, you can build the
		WorldViewProjection matrix that you then pass to that function.
		Hint, you’ll have to reinterpret the Matrix data...
		*/
		m_pMatWorldViewProjVariable->SetMatrix(reinterpret_cast<const float*>(&matrix));
	}

	void Effect::SetWorldMatrix(const Matrix& matrix)
	{
		
	}

	void Effect::SetInverseViewMatrix(const Matrix& matrix)
	{
		
	}

	void Effect::SetDiffuseMap(Texture* pDiffuseTexture)
	{
		if (m_pDiffuseMapVariable)
		{
			m_pDiffuseMapVariable->SetResource(pDiffuseTexture->GetShaderResourceView());
		}
	}

	void Effect::SetFilteringMethod(FilteringMethod filterMethod)
	{
		switch (filterMethod)
		{
		case dae::Effect::FilteringMethod::Point:
			m_pTechnique = m_pEffect->GetTechniqueByName("PointFilteringTechnique");
			if (!m_pTechnique->IsValid()) std::wcout << L"PointTechnique not valid\n";
			break;
		case dae::Effect::FilteringMethod::Linear:
			m_pTechnique = m_pEffect->GetTechniqueByName("LinearFilteringTechnique");
			if (!m_pTechnique->IsValid()) std::wcout << L"LinearTechnique not valid\n";
			break;
		case dae::Effect::FilteringMethod::Anisotropic:
			m_pTechnique = m_pEffect->GetTechniqueByName("AnisotropicFilteringTechnique");
			if (!m_pTechnique->IsValid()) std::wcout << L"AnisotropicTechnique not valid\n";
			break;
		}
	}

	void Effect::SetCullingMode(CullingMode cullMode)
	{
		D3D11_RASTERIZER_DESC rasterizerDesc{};
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.FrontCounterClockwise = false;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.DepthClipEnable = true;
		rasterizerDesc.ScissorEnable = false;
		rasterizerDesc.MultisampleEnable = false;
		rasterizerDesc.AntialiasedLineEnable = false;

		switch (cullMode)
		{
		case CullingMode::Front:
		{
			rasterizerDesc.CullMode = D3D11_CULL_FRONT;
			break;
		}
		case CullingMode::Back:
		{
			rasterizerDesc.CullMode = D3D11_CULL_BACK;
			break;
		}
		case CullingMode::None:
		{
			rasterizerDesc.CullMode = D3D11_CULL_NONE;
			break;
		}
		}

		if (m_pRasterizerState) m_pRasterizerState->Release();

		HRESULT hr{ m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pRasterizerState) };
		if (FAILED(hr))
		{
			std::cout << "m_pRasterizerState failed to load\n";
		}

		hr = m_pRasterizerStateVariable->SetRasterizerState(0, m_pRasterizerState);
		if (FAILED(hr))
		{
			std::cout << "Failed to change rasterizer state\n";
		}
	}

	ID3DX11Effect* Effect::LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile)
	{
		HRESULT result;
		ID3D10Blob* pErrorBlob{ nullptr };
		ID3DX11Effect* pEffect;

		DWORD shaderFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
		shaderFlags |= D3DCOMPILE_DEBUG;
		shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		result = D3DX11CompileEffectFromFile
		(
			assetFile.c_str(),
			nullptr,
			nullptr,
			shaderFlags,
			0,
			pDevice,
			&pEffect,
			&pErrorBlob
		);

		if (FAILED(result))
		{
			if (pErrorBlob != nullptr)
			{
				const char* pErrors{ static_cast<char*>(pErrorBlob->GetBufferPointer()) };

				std::wstringstream ss;
				for (unsigned int i{}; i < pErrorBlob->GetBufferSize(); ++i)
				{
					ss << pErrors[i];
				}

				OutputDebugStringW(ss.str().c_str());
				pErrorBlob->Release();
				pErrorBlob = nullptr;

				std::wcout << ss.str() << "\n";
			}
			else
			{
				std::wstringstream ss;
				ss << "EffectLoader: Failed to CreateEffectFromFile!\nPath: " << assetFile;
				std::wcout << ss.str() << "\n";
				return nullptr;
			}
		}

		return pEffect;
	}
}