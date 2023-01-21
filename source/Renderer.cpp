#include "pch.h"
#include "Renderer.h"

#include "Mesh.h"
#include "ShadedEffect.h"
#include "Texture.h"

#include "HelperFuncts.h"
#include "Utils.h"

namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) 
		:m_pWindow(pWindow)
	{
		m_pCurrentRendererfunction = [this] {Render_hardware(); };
		m_IsUsingHardware = true;

		//----------------------------------------------
		// Initialize
		//----------------------------------------------
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//----------------------------------------------
		// Initialize DirectX pipeline
		//----------------------------------------------
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

		//----------------------------------------------
		// Initialize Software pipeline
		//----------------------------------------------
		//Create Buffers
		m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
		m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
		m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

		m_pDepthBufferPixels = new float[m_Width * m_Height];
		ResetDepthBuffer();

		//----------------------------------------------
		// Initialize Camera
		//----------------------------------------------
		m_Camera.Initialize(45.f, {}, static_cast<float>(m_Width) / m_Height);

		//----------------------------------------------
		// Initialize meshes
		//----------------------------------------------
		auto pShadedEffect{ std::make_unique<ShadedEffect>(m_pDevice, L"Resources/PosCol3D.fx") };

		m_pVehicleDiffuseTexture = std::make_unique<Texture>( m_pDevice, "Resources/vehicle_diffuse.png" );
		m_pVehicleNormalTexture = std::make_unique<Texture>( m_pDevice, "Resources/vehicle_normal.png" );
		m_pVehicleSpecularTexture = std::make_unique<Texture>( m_pDevice, "Resources/vehicle_specular.png" );
		m_pVehicleGlossinessTexture = std::make_unique<Texture>( m_pDevice, "Resources/vehicle_gloss.png" );
		pShadedEffect->SetDiffuseMap(m_pVehicleDiffuseTexture.get());
		pShadedEffect->SetNormalMap(m_pVehicleNormalTexture.get());
		pShadedEffect->SetSpecularMap(m_pVehicleSpecularTexture.get());
		pShadedEffect->SetGlossinessMap(m_pVehicleGlossinessTexture.get());

		m_pMeshes.push_back(new Mesh{ m_pDevice, "Resources/vehicle.obj", std::move(pShadedEffect) });

		auto pEffect{ std::make_unique<Effect>(m_pDevice, L"Resources/Transparent3D.fx") };

		Texture fireDiffuseTexture{ m_pDevice, "Resources/fireFX_diffuse.png" };
		pEffect->SetDiffuseMap(&fireDiffuseTexture);

		Mesh* pFireFX = new Mesh{ m_pDevice, "Resources/fireFX.obj",std::move(pEffect) };
		m_pFireFX = pFireFX;
		m_pMeshes.push_back(pFireFX);

		for (auto& pMesh : m_pMeshes)
		{
			pMesh->Translate(0, 0, 50);
		}
	}

	Renderer::~Renderer()
	{
		for (auto& pMesh : m_pMeshes)
		{
			delete pMesh;
			pMesh = nullptr;
		}
		m_pMeshes.clear();

		SAFE_DELETE_ARR(m_pDepthBufferPixels);

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

		for (auto& pMesh : m_pMeshes)
		{
			if (m_EnableRotation)
			{
				pMesh->RotateY(m_RotationSpeed * pTimer->GetElapsed() * TO_RADIANS);
			}
			pMesh->UpdateViewMatrices(m_Camera.GetWorldViewProjection(), m_Camera.GetInverseViewMatrix());
		}
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
		std::cout << RED << "[GLOBAL MODE] " << (m_IsUsingHardware ? "Hardware" : "Software") << '\n' << RESET;
	}

	void Renderer::ToggleRotation()
	{
		m_EnableRotation = !m_EnableRotation;
		std::cout << YELLOW << "[ROTATION] " << (m_EnableRotation ? "Enabled" : "Disabled") << '\n' << RESET;
	}

	void Renderer::CycleCullModes()
	{
		std::cout << "[CULLMODE] Not implemented yet!\n";
	}

	void Renderer::ToggleUniformClearColor()
	{
		m_EnableUniformClearColor = !m_EnableUniformClearColor;
		std::cout << GREEN << "[UNIFORM COLOR] " << (m_EnableUniformClearColor ? "Enabled" : "Disabled") << '\n' << RESET;
	}

	void Renderer::ToggleFireFXMesh()
	{
		m_EnableFireFX = !m_EnableFireFX;
		std::cout << "[FIREFX] " << (m_EnableFireFX ? "Enabled" : "Disabled") << '\n';
	}

	void Renderer::ToggleTextureSamplingStates()
	{
		m_FilteringMethod = static_cast<Effect::FilteringMethod>((static_cast<int>(m_FilteringMethod) + 1) % (static_cast<int>(Effect::FilteringMethod::END)));
		for (const auto& pMesh : m_pMeshes)
		{
			pMesh->SetFilteringMethod(m_FilteringMethod);
		}

		std::cout << "[FILTERINGMETHOD] ";
		switch (m_FilteringMethod)
		{
		case dae::Effect::FilteringMethod::Point:
			std::cout << "Point\n";
			break;
		case dae::Effect::FilteringMethod::Linear:
			std::cout << "Linear\n";
			break;
		case dae::Effect::FilteringMethod::Anisotropic:
			std::cout << "Anisotropic\n";
			break;
		}
	}

	void Renderer::CycleShadingMode()
	{
		m_ShadingMode = static_cast<ShadingMode>((static_cast<int>(m_ShadingMode) + 1) % (static_cast<int>(ShadingMode::END)));

		std::cout << "[SHADINGMODE] ";
		switch (m_ShadingMode)
		{
		case dae::Renderer::ShadingMode::ObservedArea:
			std::cout << "Observed Area\n";
			break;
		case dae::Renderer::ShadingMode::Diffuse:
			std::cout << "Diffuse\n";
			break;
		case dae::Renderer::ShadingMode::Specular:
			std::cout << "Specular\n";
			break;
		case dae::Renderer::ShadingMode::Combined:
			std::cout << "Combined\n";
			break;
		}

	}

	void Renderer::ToggleNormalMap()
	{
		m_EnableNormalMap = !m_EnableNormalMap;
		std::cout << "[NORMAL MAP] " << (m_EnableNormalMap ? "Enabled" : "Disabled") << '\n';
	}

	void Renderer::ToggleDepthBufferVisualisation()
	{
		m_EnableDepthBufferVisualisation = !m_EnableDepthBufferVisualisation;
		std::cout << "[DEPTHBUFFER VISUALISATION] " << (m_EnableDepthBufferVisualisation ? "Enabled" : "Disabled") << '\n';
	}

	void Renderer::ToggleBoundingBoxVisualisation()
	{
		m_EnableBoundingBoxVisualisation = !m_EnableBoundingBoxVisualisation;
		std::cout << "[BOUNDINGBOX VISUALISATION] " << (m_EnableBoundingBoxVisualisation ? "Enabled" : "Disabled") << '\n';
	}

	void Renderer::Render_software() const
	{
		//@START
		//Lock BackBuffer
		SDL_LockSurface(m_pBackBuffer);

		// Define Triangles - Vertices in WORLD space
		std::vector<UntexturedMesh> meshes_world;
		for (const auto& pMesh : m_pMeshes)
		{
			if (pMesh == m_pFireFX) continue;

			UntexturedMesh uMesh;
			uMesh.indices = pMesh->indices;
			uMesh.primitiveTopology = pMesh->primitiveTopology;
			uMesh.vertices = pMesh->vertices;
			uMesh.vertices_out = pMesh->vertices_out;
			uMesh.worldMatrix = pMesh->worldMatrix;
			meshes_world.emplace_back(uMesh);
		}

		// For each mesh
		for (auto& mesh : meshes_world)
		{
			// World space --> NDC Space
			VertexTransformationFunction(mesh);

			std::vector<Vector2> vertices_raster;
			for (const Vertex_Out& ndcVertex : mesh.vertices_out)
			{
				// Formula from slides
				// NDC --> Screenspace
				vertices_raster.push_back({ (ndcVertex.position.x + 1) / 2.0f * m_Width, (1.0f - ndcVertex.position.y) / 2.0f * m_Height });
			}

			// Depth buffer
			ResetDepthBuffer();
			ClearBackground();

			// +--------------+
			// | RENDER LOGIC |
			// +--------------+
			switch (mesh.primitiveTopology)
			{
			case PrimitiveTopology::TriangleList:
				// For each triangle
				for (int currStartVertIdx{ 0 }; currStartVertIdx < mesh.indices.size(); currStartVertIdx += 3)
				{
					RenderMeshTriangle(mesh, vertices_raster, currStartVertIdx, false);
				}
				break;
			case PrimitiveTopology::TriangleStrip:
				// For each triangle
				for (int currStartVertIdx{ 0 }; currStartVertIdx < mesh.indices.size() - 2; ++currStartVertIdx)
				{
					RenderMeshTriangle(mesh, vertices_raster, currStartVertIdx, currStartVertIdx % 2);
				}
				break;
			default:
				std::cout << "PrimitiveTopology not implemented yet\n";
				break;
			}
		}

		//@END
		//Update SDL Surface
		SDL_UnlockSurface(m_pBackBuffer);
		SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void dae::Renderer::VertexTransformationFunction(UntexturedMesh& mesh) const
	{
		Matrix worldViewProjectionMatrix{ mesh.worldMatrix * m_Camera.GetWorldViewProjection() };
		mesh.vertices_out.clear();
		mesh.vertices_out.reserve(mesh.vertices.size());

		for (const Vertex& v : mesh.vertices)
		{
			Vertex_Out vertex_out{ Vector4{}, v.uv, v.normal, v.tangent, v.color };

			vertex_out.position = worldViewProjectionMatrix.TransformPoint({ v.position, 1.0f });
			vertex_out.viewDirection = Vector3{ vertex_out.position.x, vertex_out.position.y, vertex_out.position.z }.Normalized();

			vertex_out.normal = mesh.worldMatrix.TransformVector(v.normal);
			vertex_out.tangent = mesh.worldMatrix.TransformVector(v.tangent);


			const float invVw{ 1 / vertex_out.position.w };
			vertex_out.position.x *= invVw;
			vertex_out.position.y *= invVw;
			vertex_out.position.z *= invVw;

			// emplace back because we made vOut just to store in this vector
			mesh.vertices_out.emplace_back(vertex_out);
		}
	}

	void dae::Renderer::RenderMeshTriangle(const UntexturedMesh& mesh, const std::vector<Vector2>& vertices_raster, int currStartVertIdx, bool swapVertices) const
	{
		const size_t vertIdx0{ mesh.indices[currStartVertIdx + (2 * swapVertices)] };
		const size_t vertIdx1{ mesh.indices[currStartVertIdx + 1] };
		const size_t vertIdx2{ mesh.indices[currStartVertIdx + (!swapVertices * 2)] };

		// If a triangle has the same vertex twice, it means it has no surface and can't be rendered.
		if (vertIdx0 == vertIdx1 || vertIdx1 == vertIdx2 || vertIdx2 == vertIdx0)
		{
			return;
		}
		if (m_Camera.ShouldVertexBeClipped(mesh.vertices_out[vertIdx0].position) || m_Camera.ShouldVertexBeClipped(mesh.vertices_out[vertIdx1].position) || m_Camera.ShouldVertexBeClipped(mesh.vertices_out[vertIdx2].position))
		{
			return;
		}

		const Vector2 vert0{ vertices_raster[vertIdx0] };
		const Vector2 vert1{ vertices_raster[vertIdx1] };
		const Vector2 vert2{ vertices_raster[vertIdx2] };

		// Boundingbox (bb)
		Vector2 bbTopLeft{ Vector2::Min(vert0,Vector2::Min(vert1,vert2)) };
		Vector2 bbBotRight{ Vector2::Max(vert0,Vector2::Max(vert1,vert2)) };

		const float margin{ 1.f };
		// Add margin to prevent seethrough lines between quads
		{
			const Vector2 marginVect{ margin,margin };
			bbTopLeft -= marginVect;
			bbBotRight += marginVect;
		}

		// Make sure the boundingbox is on the screen
		bbTopLeft.x = Clamp(bbTopLeft.x, 0.f, static_cast<float>(m_Width));
		bbTopLeft.y = Clamp(bbTopLeft.y, 0.f, static_cast<float>(m_Height));
		bbBotRight.x = Clamp(bbBotRight.x, 0.f, static_cast<float>(m_Width));
		bbBotRight.y = Clamp(bbBotRight.y, 0.f, static_cast<float>(m_Height));

		const int startX{ static_cast<int>(bbTopLeft.x) };
		const int endX{ static_cast<int>(bbBotRight.x) };
		const int startY{ static_cast<int>(bbTopLeft.y) };
		const int endY{ static_cast<int>(bbBotRight.y) };

		// For each pixel
		for (int px{ startX }; px < endX; ++px)
		{
			for (int py{ startY }; py < endY; ++py)
			{
				if (m_EnableBoundingBoxVisualisation)
				{
					ColorRGB finalColor{ 1.f,1.f,1.f };
					//Update Color in Buffer

					m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColor.r * 255),
						static_cast<uint8_t>(finalColor.g * 255),
						static_cast<uint8_t>(finalColor.b * 255));
					continue;
				}

				const Vector2 currentPixel{ static_cast<float>(px),static_cast<float>(py) };
				const int pixelIdx{ px + py * m_Width };
				// Cross products for weights go to waste, optimalisation is possible
				const bool hitTriangle{ Utils::IsInTriangle(currentPixel,vert0,vert1,vert2) };
				if (hitTriangle)
				{

					// weights
					float weight0, weight1, weight2;
					weight0 = Vector2::Cross((currentPixel - vert1), (vert1 - vert2));
					weight1 = Vector2::Cross((currentPixel - vert2), (vert2 - vert0));
					weight2 = Vector2::Cross((currentPixel - vert0), (vert0 - vert1));
					// divide by total triangle area
					const float totalTriangleArea{ Vector2::Cross(vert1 - vert0,vert2 - vert0) };
					const float invTotalTriangleArea{ 1 / totalTriangleArea };
					weight0 *= invTotalTriangleArea;
					weight1 *= invTotalTriangleArea;
					weight2 *= invTotalTriangleArea;

					const float depth0{ mesh.vertices_out[vertIdx0].position.z };
					const float depth1{ mesh.vertices_out[vertIdx1].position.z };
					const float depth2{ mesh.vertices_out[vertIdx2].position.z };
					const float interpolatedDepth{ 1.f / (weight0 * (1.f / depth0) + weight1 * (1.f / depth1) + weight2 * (1.f / depth2)) };
					if (m_pDepthBufferPixels[pixelIdx] < interpolatedDepth || interpolatedDepth < 0.f || interpolatedDepth > 1.f) continue;

					m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

					Vertex_Out pixel{};
					pixel.position = { currentPixel.x,currentPixel.y, interpolatedDepth,interpolatedDepth };
					pixel.uv = interpolatedDepth * ((weight0 * mesh.vertices[vertIdx0].uv) / depth0 + (weight1 * mesh.vertices[vertIdx1].uv) / depth1 + (weight2 * mesh.vertices[vertIdx2].uv) / depth2);
					pixel.normal = Vector3{ interpolatedDepth * (weight0 * mesh.vertices_out[vertIdx0].normal / mesh.vertices_out[vertIdx0].position.w + weight1 * mesh.vertices_out[vertIdx1].normal / mesh.vertices_out[vertIdx1].position.w + weight2 * mesh.vertices_out[vertIdx2].normal / mesh.vertices_out[vertIdx2].position.w) }.Normalized();
					pixel.tangent = Vector3{ interpolatedDepth * (weight0 * mesh.vertices_out[vertIdx0].tangent / mesh.vertices_out[vertIdx0].position.w + weight1 * mesh.vertices_out[vertIdx1].tangent / mesh.vertices_out[vertIdx1].position.w + weight2 * mesh.vertices_out[vertIdx2].tangent / mesh.vertices_out[vertIdx2].position.w) }.Normalized();
					pixel.viewDirection = Vector3{ interpolatedDepth * (weight0 * mesh.vertices_out[vertIdx0].viewDirection / mesh.vertices_out[vertIdx0].position.w + weight1 * mesh.vertices_out[vertIdx1].viewDirection / mesh.vertices_out[vertIdx1].position.w + weight2 * mesh.vertices_out[vertIdx2].viewDirection / mesh.vertices_out[vertIdx2].position.w) }.Normalized();

					PixelShading(pixel);
				}
			}
		}
	}

	void dae::Renderer::PixelShading(const Vertex_Out& v) const
	{
		Vector3 normal{ v.normal };

		ColorRGB finalColor{};

		if (m_EnableNormalMap)
		{
			const Vector3 binormal = Vector3::Cross(v.normal, v.tangent);
			const Matrix tangentSpaceAxis = Matrix{ v.tangent,binormal,v.normal,Vector3::Zero };

			const ColorRGB normalSampleVecCol{ (2 * m_pVehicleNormalTexture->Sample(v.uv)) - ColorRGB{1,1,1} };
			const Vector3 normalSampleVec{ normalSampleVecCol.r,normalSampleVecCol.g,normalSampleVecCol.b };
			normal = tangentSpaceAxis.TransformVector(normalSampleVec);
		}

		if (!m_EnableDepthBufferVisualisation)
		{
			const float observedArea{ Vector3::DotClamp(normal.Normalized(), -m_GlobalLight.direction) };
			finalColor = m_pVehicleDiffuseTexture->Sample(v.uv);
			const ColorRGB lambert{ BRDF::Lambert(1.0f, m_pVehicleDiffuseTexture->Sample(v.uv)) };
			const float specularVal{ m_SpecularShininess * m_pVehicleGlossinessTexture->Sample(v.uv).r };
			const ColorRGB specular{ m_pVehicleSpecularTexture->Sample(v.uv) * BRDF::Phong(1.0f, specularVal, -m_GlobalLight.direction, v.viewDirection, normal) };

			// += since finalColor is already a sample of the diffuse texture
			switch (m_ShadingMode)
			{
			case dae::Renderer::ShadingMode::ObservedArea:
				finalColor = ColorRGB{ observedArea, observedArea, observedArea };
				break;
			case dae::Renderer::ShadingMode::Diffuse:
				finalColor += m_GlobalLight.intensity * observedArea * lambert;
				break;
			case dae::Renderer::ShadingMode::Specular:
				finalColor += specular * observedArea;
				break;
			case dae::Renderer::ShadingMode::Combined:
				finalColor += m_GlobalLight.intensity * observedArea * lambert + specular;
				break;
			}

		}
		else
		{
			const float depthCol{ Utils::Remap(v.position.w,0.985f,1.f) };
			finalColor = { depthCol,depthCol,depthCol };
		}

		//Update Color in Buffer
		finalColor.MaxToOne();

		const int px{ static_cast<int>(v.position.x) };
		const int py{ static_cast<int>(v.position.y) };

		m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
			static_cast<uint8_t>(finalColor.r * 255),
			static_cast<uint8_t>(finalColor.g * 255),
			static_cast<uint8_t>(finalColor.b * 255));
	}

	void Renderer::Render_hardware() const
	{
		// 1. Clear RTV and DSV
		ColorRGB clearColor{ (m_EnableUniformClearColor ? m_UniformClearColor : m_HardwareClearColor) };

		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, &clearColor.r);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		// 2. Set pipeline + Invoke drawcalls (= render)
		for (const auto& pMesh : m_pMeshes)
		{
			if (m_EnableFireFX && pMesh == m_pFireFX) continue;
			pMesh->Render(m_pDeviceContext);
		}

		// 3. Present backbuffer (swap)
		m_pSwapChain->Present(0, 0);
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
