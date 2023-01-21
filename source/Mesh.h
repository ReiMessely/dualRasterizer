#pragma once
#include "DataTypes.h"
#include "Effect.h"

enum class PrimitiveTopology
{
	TriangleList,
	TriangleStrip
};

namespace dae
{
	class UntexturedMesh
	{
	public:
		// Software
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleList };

		std::vector<Vertex_Out> vertices_out{};

		Matrix worldMatrix;

		inline void RotateX(float angle)
		{
			worldMatrix = Matrix::CreateRotationX(angle) * worldMatrix;
		}

		inline void RotateY(float angle)
		{
			worldMatrix = Matrix::CreateRotationY(angle) * worldMatrix;
		}

		inline void RotateZ(float angle)
		{
			worldMatrix = Matrix::CreateRotationZ(angle) * worldMatrix;
		}

		inline void Translate(const Vector3& v)
		{
			Translate(v.x, v.y, v.z);
		}

		inline void Translate(float x, float y, float z)
		{
			worldMatrix = Matrix::CreateTranslation(x, y, z) * worldMatrix;
		}
	};

	class Mesh final : public UntexturedMesh
	{
	public:
		Mesh(ID3D11Device* pDevice, const std::string& objFilePath, std::unique_ptr<Effect> pEffect);
		~Mesh();

		Mesh(const Mesh& other) = delete;
		Mesh& operator=(const Mesh& other) = delete;
		Mesh(Mesh&& other) = delete;
		Mesh& operator=(Mesh&& other) = delete;

		// Hardware
		void Render(ID3D11DeviceContext* pDeviceContext) const;

		void UpdateViewMatrices(const Matrix& viewProjectionMatrix, const Matrix& inverseViewMatrix);

		void SetFilteringMethod(Effect::FilteringMethod filteringMethod);
	private:
		std::unique_ptr<Effect> m_pEffect{};

		ID3D11InputLayout* m_pInputLayout{};
		ID3D11Buffer* m_pVertexBuffer{};
		ID3D11Buffer* m_pIndexBuffer{};
	};

}