#pragma once
#include <functional>
#include "Camera.h"
#include "Effect.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class UntexturedMesh;
	class Mesh;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(const Timer* pTimer);
		void Render() const;

		// ------ SHARED ------
		//
		// F1
		void ToggleBetweenHardwareSoftware();
		// F2
		void ToggleRotation();
		// F9 
		void CycleCullModes();
		// F10
		void ToggleUniformClearColor();
		// F11
		// TogglePrintFPS is found in the main.cpp

		// ------ HARDWARE ONLY ------
		//
		// F3
		void ToggleFireFXMesh();
		// F4
		void ToggleTextureSamplingStates();

		// ------ SOFTWARE ONLY ------
		//
		// F5
		void CycleShadingMode();
		// F6
		void ToggleNormalMap();
		// F7
		void ToggleDepthBufferVisualisation();
		// F8
		void ToggleBoundingBoxVisualisation();

	private:
		// Base
		SDL_Window* m_pWindow{};

		int m_Width{};
		int m_Height{};

		bool m_IsInitialized{ false };

		const ColorRGB m_HardwareClearColor{ .39f, .59f, .93f };
		const ColorRGB m_SoftwareClearColor{ .39f, .39f, .39f };
		const ColorRGB m_UniformClearColor{ .1f, .1f, .1f };

		// Mode switch
		bool m_IsUsingHardware;
		std::function<void()> m_pCurrentRendererfunction;

		bool m_EnableRotation{ true };
		bool m_EnableUniformClearColor{ false };
		bool m_EnableFireFX{ true };
		const Mesh* m_pFireFX{ nullptr };
		bool m_EnableNormalMap{ true };
		bool m_EnableDepthBufferVisualisation{ false };
		bool m_EnableBoundingBoxVisualisation{ false };
		Effect::FilteringMethod m_FilteringMethod{ Effect::FilteringMethod::Point };
		CullingMode m_CullingMode{ CullingMode::Back };
		// Shading method is under software

		// ...
		Camera m_Camera;
		std::vector<Mesh*> m_pMeshes;

		float m_RotationSpeed{ 45.f }; // in degrees per second

		//Render methods
		void Render_software() const;
		void Render_hardware() const;

		// Software
		enum class ShadingMode
		{
			ObservedArea,	// (OA)
			Diffuse,		// (incl OA)
			Specular,		// (incl OA)
			Combined,
			END
		};
		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		std::unique_ptr<Texture> m_pVehicleDiffuseTexture;
		std::unique_ptr<Texture> m_pVehicleNormalTexture;
		std::unique_ptr<Texture> m_pVehicleSpecularTexture;
		std::unique_ptr<Texture> m_pVehicleGlossinessTexture;

		ShadingMode m_ShadingMode{ ShadingMode::Combined };

		// Needs to be paired with HARDWARE
		const DirectionalLight m_GlobalLight{ Vector3{ .577f,-.557f,.577f }.Normalized() , 7.f };
		const float m_SpecularShininess{ 25.0f };
		const ColorRGB m_AmbientColor{ 0.025f, 0.025f, 0.025f };

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(UntexturedMesh& mesh) const;
		// std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX);
		inline void ResetDepthBuffer() const { std::fill_n(m_pDepthBufferPixels, (m_Width * m_Height), FLT_MAX); }
		// SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, r, g, b ));
		inline void ClearBackground() const 
		{ 
			Uint8 r, g, b;
			ColorRGB clearColor{ (m_EnableUniformClearColor ? m_UniformClearColor : m_SoftwareClearColor) };
			clearColor *= 255;
			r = static_cast<Uint8>(clearColor.r);
			g = static_cast<Uint8>(clearColor.g);
			b = static_cast<Uint8>(clearColor.b);
			SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, r, g, b)); 
		}

		void RenderMeshTriangle(const UntexturedMesh& mesh, const std::vector<Vector2>& vertices_raster, int currentVertexIdx, bool swapVertices) const;

		void PixelShading(const Vertex_Out& v) const;

		//DIRECTX
		HRESULT InitializeDirectX();
		
		ID3D11Device* m_pDevice{};
		ID3D11DeviceContext* m_pDeviceContext{};
		IDXGISwapChain* m_pSwapChain{};
		ID3D11Texture2D* m_pDepthStencilBuffer{};
		ID3D11DepthStencilView* m_pDepthStencilView{};
		ID3D11Resource* m_pRenderTargetBuffer{};
		ID3D11RenderTargetView* m_pRenderTargetView{};
	};
}
