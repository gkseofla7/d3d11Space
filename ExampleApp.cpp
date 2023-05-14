#include "ExampleApp.h"

#include <directxtk/DDSTextureLoader.h> // 큐브맵 읽을 때 필요
#include <tuple>
#include <vector>

#include "GeometryGenerator.h"

namespace hlab {

using namespace std;
using namespace DirectX;

ExampleApp::ExampleApp() : AppBase() {}

bool ExampleApp::Initialize() {

    if (!AppBase::Initialize())
        return false;


    m_cubeMapping.Initialize(m_device,
                             L"./CubemapTextures/Ice.dds",
                             L"./CubemapTextures/Ice.dds");

    MeshData sphere = GeometryGenerator::MakeSphere(0.3f, 100, 100);
    sphere.textureFilename = "ojwD8.jpg";
    m_meshGroupSphere.Initialize(m_device, {sphere});
    m_meshGroupSphere.m_diffuseResView = m_cubeMapping.m_diffuseResView;
    m_meshGroupSphere.m_specularResView = m_cubeMapping.m_specularResView;

    m_meshGroupCharacter.Initialize(m_device, "C:/Users/DaerimHan/Desktop/d3d11/Zelda/", "zeldaPosed001.fbx");
    m_meshGroupCharacter.m_diffuseResView = m_cubeMapping.m_diffuseResView;
    m_meshGroupCharacter.m_specularResView = m_cubeMapping.m_specularResView;

     MeshData sun = GeometryGenerator::MakeSphere(0.3f, 100, 100);
    sun.textureFilename = "shadertoytexture0.jpg";
     m_meshSun.Initialize(m_device, {sun}); // "StarPixelShader.hlsl"
    m_meshSun.m_radius = 0.3f;
     m_meshSun.m_diffuseResView = m_cubeMapping.m_diffuseResView;
    m_meshSun.m_specularResView = m_cubeMapping.m_specularResView;
    BuildFilters();

    return true;
}

void ExampleApp::Update(float dt) {

    using namespace DirectX;
    static float time = 0.0f;
    auto &visibleMeshGroup =
        m_visibleMeshIndex == 0 ? m_meshGroupSphere : m_meshGroupCharacter;

    auto modelRow = Matrix::CreateScale(m_modelScaling) *
                    Matrix::CreateRotationY(m_modelRotation.y) *
                    Matrix::CreateRotationX(m_modelRotation.x) *
                    Matrix::CreateRotationZ(m_modelRotation.z) *
                    Matrix::CreateTranslation(m_modelTranslation);
    modelRow = modelRow*Matrix::CreateRotationY(m_worldRotation.y * time) *
               Matrix::CreateRotationZ(m_worldRotation.z * time) ;
    auto invTransposeRow = modelRow;
    invTransposeRow.Translation(Vector3(0.0f));
    invTransposeRow = invTransposeRow.Invert().Transpose();

    auto viewRow = Matrix::CreateRotationY(m_viewRot.y) *
                   Matrix::CreateRotationX(m_viewRot.x) *
                   Matrix::CreateTranslation(0.0f, 0.0f, 2.0f);

    const float aspect = AppBase::GetAspectRatio();
    Matrix projRow =
        m_usePerspectiveProjection
            ? XMMatrixPerspectiveFovLH(XMConvertToRadians(m_projFovAngleY),
                                       aspect, m_nearZ, m_farZ)
            : XMMatrixOrthographicOffCenterLH(-aspect, aspect, -1.0f, 1.0f,
                                              m_nearZ, m_farZ);
    //Sun 관련 Update
    m_meshSun.m_basicVertexConstantData.model = modelRow.Transpose();
    m_meshSun.m_basicVertexConstantData.view = viewRow.Transpose();
    m_meshSun.m_basicVertexConstantData.projection = projRow.Transpose();

    auto eyeWorld = Vector3::Transform(Vector3(0.0f), viewRow.Invert());
    Vector3 centerWorldPos = Vector3::Transform(m_meshSun.m_center, modelRow);
    Matrix projSun = Matrix::CreateLookAt(centerWorldPos, eyeWorld, Vector3(0.f, 1.f, 0.f));
    m_meshSun.m_basicPixelConstantData.sunViewMatrix = projSun.Transpose();
    m_meshSun.m_basicPixelConstantData.sphereRadius = m_meshSun.m_radius;
    m_meshSun.m_basicPixelConstantData.iTime = time;
    m_meshSun.m_basicPixelConstantData.eyeWorld = eyeWorld;
    m_meshSun.UpdateConstantBuffers(m_device, m_context);

    //여긴 일단 무시
    {
        // MeshGroup의 ConstantBuffers 업데이트
        for (int i = 0; i < MAX_LIGHTS; i++) {
            // 다른 조명 끄기
            if (i != m_lightType) {
                visibleMeshGroup.m_basicPixelConstantData.lights[i].strength *=
                    0.0f;
            } else {
                visibleMeshGroup.m_basicPixelConstantData.lights[i] =
                    m_lightFromGUI;
            }
        }
        {
            visibleMeshGroup.m_basicPixelConstantData.eyeWorld = eyeWorld;
            visibleMeshGroup.m_basicPixelConstantData.material.diffuse =
                Vector3(m_materialDiffuse);
            visibleMeshGroup.m_basicPixelConstantData.material.specular =
                Vector3(m_materialSpecular);
        }
        visibleMeshGroup.m_basicVertexConstantData.model = modelRow.Transpose();
        visibleMeshGroup.m_basicVertexConstantData.view = viewRow.Transpose();
        visibleMeshGroup.m_basicVertexConstantData.projection =
            projRow.Transpose();

        visibleMeshGroup.m_basicPixelConstantData.eyeWorld = eyeWorld;

        visibleMeshGroup.m_basicPixelConstantData.material.diffuse =
            Vector3(m_materialDiffuse);
        visibleMeshGroup.m_basicPixelConstantData.material.specular =
            Vector3(m_materialSpecular);
        visibleMeshGroup.UpdateConstantBuffers(m_device, m_context);
    }
    // 큐브 매핑 Constant Buffer 업데이트
    m_cubeMapping.UpdateConstantBuffers(m_device, m_context,
                                        (Matrix::CreateRotationY(m_viewRot.y) *
                                         Matrix::CreateRotationX(m_viewRot.x))
                                            .Transpose(),
                                        projRow.Transpose());

    //if (m_dirtyflag) {
    //    m_filters[1]->m_pixelConstData.threshold = m_threshold;
    //    m_filters[1]->UpdateConstantBuffers(m_device, m_context);
    //    m_filters.back()->m_pixelConstData.strength = m_strength;
    //    m_filters.back()->UpdateConstantBuffers(m_device, m_context);

    //    m_dirtyflag = 0;
    //}
    time += dt;
}

void ExampleApp::Render() {

    // RS: Rasterizer stage
    // OM: Output-Merger stage
    // VS: Vertex Shader
    // PS: Pixel Shader
    // IA: Input-Assembler stage

    SetViewport();

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(),
                                     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                     1.0f, 0);
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
                                  m_depthStencilView.Get());
    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

    if (m_drawAsWire) {
        m_context->RSSetState(m_wireRasterizerSate.Get());
    } else {
        m_context->RSSetState(m_rasterizerSate.Get());
    }
    // 큐브매핑
    m_cubeMapping.Render(m_context);
    m_meshSun.Render(m_context);
    // 물체들
    //if (m_visibleMeshIndex == 0) {
    //    m_meshGroupSphere.Render(m_context);
    //} else {
    //    m_meshGroupCharacter.Render(m_context);
    //}
    

    // 후처리 필터 시작하기 전에 Texture2DMS에 렌더링 된 결과를 Texture2D로 복사
    ComPtr<ID3D11Texture2D> backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf()));
    m_context->ResolveSubresource(m_tempTexture.Get(), 0, backBuffer.Get(), 0,
                                  DXGI_FORMAT_R8G8B8A8_UNORM);
    // 후처리 필터
    for (auto &f : m_filters) {
        f->Render(m_context);
    }
}

void ExampleApp::BuildFilters() {

    m_filters.clear();

}

void ExampleApp::UpdateGUI() {

    auto &meshGroup =
        m_visibleMeshIndex == 0 ? m_meshGroupSphere : m_meshGroupCharacter;

    if (ImGui::RadioButton("Sphere", m_visibleMeshIndex == 0)) {
        m_visibleMeshIndex = 0;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Character", m_visibleMeshIndex == 1)) {
        m_visibleMeshIndex = 1;
    }

    m_dirtyflag +=
        ImGui::SliderFloat("Bloom Threshold", &m_threshold, 0.0f, 1.0f);
    m_dirtyflag +=
        ImGui::SliderFloat("Bloom Strength", &m_strength, 0.0f, 3.0f);

    ImGui::Checkbox("Use Texture",
                    &meshGroup.m_basicPixelConstantData.useTexture);
    ImGui::Checkbox("Wireframe", &m_drawAsWire);
    ImGui::Checkbox("Draw Normals", &meshGroup.m_drawNormals);
    if (ImGui::SliderFloat("Normal scale",
                           &meshGroup.m_normalVertexConstantData.scale, 0.0f,
                           1.0f)) {
        meshGroup.m_drawNormalsDirtyFlag = true;
    }

    ImGui::SliderFloat3("m_modelRotation", &m_modelRotation.x, -3.14f, 3.14f);
    ImGui::SliderFloat3("m_viewRot", &m_viewRot.x, -3.14f, 3.14f);
    ImGui::SliderFloat3(
        "Material FresnelR0",
        &meshGroup.m_basicPixelConstantData.material.fresnelR0.x, 0.0f, 1.0f);

    ImGui::SliderFloat("Material Diffuse", &m_materialDiffuse, 0.0f, 3.0f);
    ImGui::SliderFloat("Material Specular", &m_materialSpecular, 0.0f, 3.0f);
    ImGui::SliderFloat("Material Shininess",
                       &meshGroup.m_basicPixelConstantData.material.shininess,
                       0.01f, 20.0f);
}

} // namespace hlab
