#pragma once

namespace dae
{
	class Texture;

	class Effect
	{
	public:
		Effect(ID3D11Device* pDevice, const std::wstring& assetFile);
		virtual ~Effect();

		Effect(const Effect& other) = delete;
		Effect& operator=(const Effect& other) = delete;
		Effect(Effect&& other) = delete;
		Effect& operator=(Effect&& other) = delete;

		enum class FilteringMethod
		{
			Point, Linear, Anisotropic, END
		};

		ID3DX11Effect* GetEffect() const;
		ID3DX11EffectTechnique* GetTechnique() const;

		void SetWorldViewProjectionMatrix(const Matrix& matrix);

		virtual void SetWorldMatrix(const Matrix& matrix);
		virtual void SetInverseViewMatrix(const Matrix& matrix);

		void SetDiffuseMap(Texture* pDiffuseTexture);

		void CycleFilteringMethods();
	protected:
		ID3DX11Effect* m_pEffect{};
		ID3DX11EffectTechnique* m_pTechnique{};

		ID3DX11EffectMatrixVariable* m_pMatWorldViewProjVariable{};

		ID3DX11EffectShaderResourceVariable* m_pDiffuseMapVariable{};

		FilteringMethod m_FilteringMethod;

		static ID3DX11Effect* LoadEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
	};
}
