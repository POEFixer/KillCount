#include "IconAtlas.h"

// stb_image implementation — define once in this TU
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include <stb_image.h>

#include <nlohmann/json.hpp>
#include <fstream>

bool IconAtlas::Initialize(void* d3dDevice) {
    auto* device = static_cast<ID3D11Device*>(d3dDevice);
    if (!device) return false;

    const char* iconPath = "Resources/radar/icons.png";
    int w = 0, h = 0;
    unsigned char* data = stbi_load(iconPath, &w, &h, nullptr, 4);
    if (!data) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = static_cast<UINT>(w);
    desc.Height = static_cast<UINT>(h);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = data;
    initData.SysMemPitch = static_cast<UINT>(w * 4);

    ID3D11Texture2D* tex = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, &initData, &tex);
    stbi_image_free(data);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = device->CreateShaderResourceView(tex, &srvDesc, &m_SRV);
    tex->Release();
    if (FAILED(hr)) return false;

    m_TextureID = reinterpret_cast<ImTextureID>(m_SRV);
    m_TexWidth = w;
    m_TexHeight = h;

    LoadIconPositions();
    return true;
}

void IconAtlas::Release() {
    if (m_SRV) {
        m_SRV->Release();
        m_SRV = nullptr;
    }
    m_TextureID = ImTextureID{};
    m_Icons.clear();
}

const IconUV& IconAtlas::GetIcon(const std::string& name) const {
    auto it = m_Icons.find(name);
    if (it != m_Icons.end()) return it->second;
    return m_EmptyIcon;
}

void IconAtlas::ComputeUV(int gridX, int gridY, float scale, IconUV& out) {
    if (m_TexWidth <= 0 || m_TexHeight <= 0) {
        out = {};
        return;
    }
    float cellU = CellSize / static_cast<float>(m_TexWidth);
    float cellV = CellSize / static_cast<float>(m_TexHeight);
    out.UV0 = ImVec2(gridX * cellU, gridY * cellV);
    out.UV1 = ImVec2((gridX + 1) * cellU, (gridY + 1) * cellV);
    out.Scale = scale;
    out.Valid = true;
}

void IconAtlas::LoadIconPositions() {
    const char* jsonPath = "Resources/radar/icons.json";
    std::ifstream f(jsonPath);
    if (!f.is_open()) {
        SetupFallbackIcons();
        return;
    }

    try {
        auto j = nlohmann::json::parse(f);
        for (const auto& item : j) {
            std::string name = item.value("name", "");
            int gx = item.value("gridX", 0);
            int gy = item.value("gridY", 0);
            float scale = item.value("scale", 30.0f);

            if (!name.empty()) {
                IconUV uv;
                ComputeUV(gx, gy, scale, uv);
                m_Icons[name] = uv;
            }
        }
    } catch (...) {
        SetupFallbackIcons();
        return;
    }

    // Add alias for "Unknown Monster" using Normal Monster icon
    if (m_Icons.find("Unknown Monster") == m_Icons.end()) {
        auto it = m_Icons.find("Normal Monster");
        if (it != m_Icons.end()) {
            m_Icons["Unknown Monster"] = it->second;
        }
    }

    // Add alias for "Death" using Self icon
    if (m_Icons.find("Death") == m_Icons.end()) {
        auto it = m_Icons.find("Self");
        if (it != m_Icons.end()) {
            m_Icons["Death"] = it->second;
        }
    }
}

void IconAtlas::SetupFallbackIcons() {
    // Hardcoded positions from icons.json
    struct FallbackIcon { const char* name; int gx; int gy; float scale; };
    static const FallbackIcon fallbacks[] = {
        { "Normal Monster",             0,  14, 30.0f },
        { "Magic Monster",              6,   3, 30.0f },
        { "Rare Monster",               4,  57, 30.0f },
        { "Unique Monster",             6,  57, 30.0f },
        { "Unknown Monster",            0,  14, 30.0f },
        { "Magic Chests",               2,  70, 25.0f },
        { "Rare Chests",                1,  70, 30.0f },
        { "Generic Expedition Chests",  4,  41, 25.0f },
        { "Breach Chest",               1,   2, 20.0f },
        { "Strongbox",                 11,  64, 30.0f },
        { "Self",                        0,   0, 15.0f },
        { "Death",                       0,   0, 15.0f },
    };

    for (const auto& fb : fallbacks) {
        IconUV uv;
        ComputeUV(fb.gx, fb.gy, fb.scale, uv);
        m_Icons[fb.name] = uv;
    }
}
