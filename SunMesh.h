#pragma once
#include "BasicConstantData.h"
#include "Mesh.h"
#include "MeshData.h"
#include "D3D11Utils.h"
namespace hlab {
class SunMesh {
  public:
    void Initialize(ComPtr<ID3D11Device> &device,
                    const MeshData &meshes);

    void UpdateConstantBuffers(ComPtr<ID3D11Device> &device,
                               ComPtr<ID3D11DeviceContext> &context);

    void Render(ComPtr<ID3D11DeviceContext> &context);

    BasicVertexConstantData m_basicVertexConstantData;
    SunPixelConstantData m_basicPixelConstantData;
    float m_radius = 0.f;
    Vector3 m_center = Vector3(0.f,0.f,0.f);
  private:
    // 메쉬 그리기
    std::vector<shared_ptr<Mesh>> m_meshes;

    ComPtr<ID3D11VertexShader> m_basicVertexShader;
    ComPtr<ID3D11PixelShader> m_basicPixelShader;
    ComPtr<ID3D11InputLayout> m_basicInputLayout;

    ComPtr<ID3D11SamplerState> m_samplerState;

    ComPtr<ID3D11Buffer> m_vertexConstantBuffer;
    ComPtr<ID3D11Buffer> m_pixelConstantBuffer;
};
} // namespace hlab