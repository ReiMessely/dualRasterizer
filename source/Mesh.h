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
	class Mesh final
	{
	public:
		Mesh(ID3D11Device* pDevice, const std::string& objFilePath, std::unique_ptr<Effect> pEffect);
		~Mesh();

		Mesh(const Mesh& other) = delete;
		Mesh& operator=(const Mesh& other) = delete;
		Mesh(Mesh&& other) = delete;
		Mesh& operator=(Mesh&& other) = delete;

		void RotateX(float angle);
		void RotateY(float angle);
		void RotateZ(float angle);
		void Translate(const Vector3& v);
		void Translate(float x, float y, float z);

		// Hardware
		void Render(ID3D11DeviceContext* pDeviceContext) const;

		void UpdateViewMatrices(const Matrix& viewProjectionMatrix, const Matrix& inverseViewMatrix);

		void SetFilteringMethod(Effect::FilteringMethod filteringMethod);

		// Software
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleList };

		std::vector<Vertex_Out> vertices_out{};
	private:
		Matrix m_WorldMatrix;

		std::unique_ptr<Effect> m_pEffect{};

		ID3D11InputLayout* m_pInputLayout{};
		ID3D11Buffer* m_pVertexBuffer{};
		ID3D11Buffer* m_pIndexBuffer{};
	};

}