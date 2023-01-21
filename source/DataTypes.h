#pragma once
#include "Math.h"
#include "vector"

namespace dae
{
	struct Vertex final
	{
		Vector3 position{};
		Vector2 uv{}; 
		Vector3 normal{}; 
		Vector3 tangent{}; 
		ColorRGB color{ colors::White };
		Vector3 viewDirection{}; 
	};

	struct Vertex_Out
	{
		Vector4 position{};
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		ColorRGB color{ colors::White };
		Vector3 viewDirection{};
	};

	struct DirectionalLight
	{
		Vector3 direction{};
		float intensity{};
	};
}