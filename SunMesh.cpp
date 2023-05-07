#include "SunMesh.h"
namespace hlab {
void SunMesh::Initialize(ComPtr<ID3D11Device> &device,
                         const std::vector<MeshData> &meshes) {

    // Sampler �����
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    device->CreateSamplerState(&sampDesc, m_samplerState.GetAddressOf());

    // ConstantBuffer �����
    m_basicVertexConstantData.model = Matrix();
    m_basicVertexConstantData.view = Matrix();
    m_basicVertexConstantData.projection = Matrix();

    D3D11Utils::CreateConstantBuffer(device, m_basicVertexConstantData,
                                     m_vertexConstantBuffer);
    D3D11Utils::CreateConstantBuffer(device, m_basicPixelConstantData,
                                     m_pixelConstantBuffer);

    for (const auto &meshData : meshes) {
        auto newMesh = std::make_shared<Mesh>();
        D3D11Utils::CreateVertexBuffer(device, meshData.vertices,
                                       newMesh->vertexBuffer);
        newMesh->m_indexCount = UINT(meshData.indices.size());
        D3D11Utils::CreateIndexBuffer(device, meshData.indices,
                                      newMesh->indexBuffer);

        if (!meshData.textureFilename.empty()) {

            std::cout << meshData.textureFilename << std::endl;
            D3D11Utils::CreateTexture(device, meshData.textureFilename,
                                      newMesh->texture,
                                      newMesh->textureResourceView);
        }

        newMesh->vertexConstantBuffer = m_vertexConstantBuffer;
        newMesh->pixelConstantBuffer = m_pixelConstantBuffer;

        this->m_meshes.push_back(newMesh);
    }

    vector<D3D11_INPUT_ELEMENT_DESC> basicInputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3D11Utils::CreateVertexShaderAndInputLayout(
        device, vsPath, basicInputElements, m_basicVertexShader,
        m_basicInputLayout);

    D3D11Utils::CreatePixelShader(device, psPath, m_basicPixelShader);

    // ��� ���� �׸���
    m_normalLines = std::make_shared<Mesh>();

    std::vector<Vertex> normalVertices;
    std::vector<uint32_t> normalIndices;

    // ���� �޽��� normal ���� �ϳ��� ��ġ��
    size_t offset = 0;
    for (const auto &meshData : meshes) {
        for (size_t i = 0; i < meshData.vertices.size(); i++) {

            auto v = meshData.vertices[i];

            v.texcoord.x = 0.0f; // ������ ǥ��
            normalVertices.push_back(v);

            v.texcoord.x = 1.0f; // ���� ǥ��
            normalVertices.push_back(v);

            normalIndices.push_back(uint32_t(2 * (i + offset)));
            normalIndices.push_back(uint32_t(2 * (i + offset) + 1));
        }
        offset += meshData.vertices.size();
    }

    D3D11Utils::CreateVertexBuffer(device, normalVertices,
                                   m_normalLines->vertexBuffer);
    m_normalLines->m_indexCount = UINT(normalIndices.size());
    D3D11Utils::CreateIndexBuffer(device, normalIndices,
                                  m_normalLines->indexBuffer);

    D3D11Utils::CreateVertexShaderAndInputLayout(
        device, L"NormalVertexShader.hlsl", basicInputElements,
        m_normalVertexShader, m_basicInputLayout);
    D3D11Utils::CreatePixelShader(device, L"NormalPixelShader.hlsl",
                                  m_normalPixelShader);

    D3D11Utils::CreateConstantBuffer(device, m_normalVertexConstantData,
                                     m_normalVertexConstantBuffer);
}

} // namespace hlab