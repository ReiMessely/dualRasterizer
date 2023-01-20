#include "pch.h"
#include "Renderer.h"

#include "HelperFuncts.h"

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) 
		:m_pWindow(pWindow)
	{
		m_pCurrentRendererfunction = [this] {Render_hardware(); };
		m_IsUsingHardware = true;

		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;
			std::cout << "DirectX is initialized and ready!\n";
		}
		else
		{
			std::cout << "DirectX initialization failed!\n";
		}

		m_Camera.Initialize(45.f, {}, static_cast<float>(m_Width) / m_Height);
	}

	Renderer::~Renderer()
	{
		SAFE_RELEASE(m_pRenderTargetView);
		SAFE_RELEASE(m_pRenderTargetBuffer);

		SAFE_RELEASE(m_pDepthStencilView);
		SAFE_RELEASE(m_pDepthStencilBuffer);

		SAFE_RELEASE(m_pSwapChain);

		if (m_pDeviceContext)
		{
			// Restore to default settings
			m_pDeviceContext->ClearState();
			// Send any queued up commands to GPU
			m_pDeviceContext->Flush();
			// Release it into the abyss
			m_pDeviceContext->Release();
		}

		SAFE_RELEASE(m_pDevice);
	}

	void Renderer::Update(const Timer* pTimer)
	{
		m_Camera.Update(pTimer);
	}


	void Renderer::Render() const
	{
		if (!m_IsInitialized)
			return;

		m_pCurrentRendererfunction();
	}

	void Renderer::ToggleBetweenHardwareSoftware()
	{
		if (m_IsUsingHardware)
		{
			m_pCurrentRendererfunction = [this] {Render_software(); };
			m_IsUsingHardware = false;
		}
		else
		{
			m_pCurrentRendererfunction = [this] {Render_hardware(); };
			m_IsUsingHardware = true;
		}
		std::cout << "[GLOBAL MODE] " << (m_IsUsingHardware ? "Hardware" : "Software") << '\n';
	}

	void Renderer::Render_software() const
	{
		
	}

	void Renderer::Render_hardware() const
	{
		
	}

	HRESULT Renderer::InitializeDirectX()
	{
		// 1. Create Device and DeviceContext
		//=======
		D3D_FEATURE_LEVEL featureLevel{ D3D_FEATURE_LEVEL_11_1 };
		uint32_t createDeviceFlags{ 0 };

#if defined(DEBUG) || defined(_DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		HRESULT result{ D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
			1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext) };
		if (FAILED(result)) return result;

		/*
		We use this factory because it adapts to whatever GPU we will be using.
		*/
		// Create DXGI Factory
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));
		if (FAILED(result))
		{
			pDxgiFactory->Release();
			return result;
		}

		/*
		Swapchain describes the fact that we will have 2 buffers to swap.
		We use the double buffer to not display a frame that is only halfly calculated.
		*/
		// 2. Create Swapchain 
		//=====
		// Description
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		// Get the handle (HWND) from the SDL backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_VERSION(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		// Create actual swapchain
		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		pDxgiFactory->Release();
		if (FAILED(result)) return result;


		// 3. Create DepthStencil (DS) and DepthStencilView (DSV)
		// Resource
		// Description
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		// View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		// Create the stencil buffer
		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result)) return result;

		// Create the stencil view
		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result)) return result;

		/*
		Now that we have our depth buffer and back buffer, I want to bind them as the active
		buffers during rendering.
		As mentioned before, binding happens through resource views. We have one for the
		depth buffer, but not for the back buffer. We can get the buffer resource from the swap
		chain using the following code. Once we have the buffer, we can create a resource view
		for it as well.
		*/
		// 4. Create RenderTarget (RT) and RenderTargetView (RTV)
		//=====

		// Resource
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result)) return result;

		// View
		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result)) return result;

		// 5. Bind RTV and DSV to Output Merger Stage
		// Using the two views, bind them as the active buffers during the Output Merger Stage.
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		/*
		Viewport defines where the back buffer will be rendered on screen. Multiple viewports
		can be handy in case of local multiplayer games or spectator views.
		Viewports are used to translate them directly to NDC space
		*/
		// 6. Set viewport
		//======
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<FLOAT>(m_Width);
		viewport.Height = static_cast<FLOAT>(m_Height);
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0;
		viewport.MaxDepth = 1;
		m_pDeviceContext->RSSetViewports(1, &viewport);

		return S_OK;
	}
}
