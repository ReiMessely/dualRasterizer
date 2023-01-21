#pragma once
#include <functional>
#include "Camera.h"
#include "Effect.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
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

		// Mode switch
		bool m_IsUsingHardware;
		std::function<void()> m_pCurrentRendererfunction;

		bool m_EnableRotation{ true };
		bool m_EnableUniformClearColor{ false };
		bool m_EnableFireFX{ true };
		bool m_EnableNormalMap{ true };
		bool m_EnableDepthBufferVisualisation{ false };
		bool m_EnableBoundingBoxVisualisation{ false };
		Effect::FilteringMethod m_FilteringMethod;

		// ...
		Camera m_Camera;
		std::vector<Mesh*> m_pMeshes;

		float m_RotationSpeed{ 30.f };

		//Render methods
		void Render_software() const;
		void Render_hardware() const;

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
