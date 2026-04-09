#pragma once

#include "imgui/imgui.h"
#include <d3d11.h>
#include <string>
#include <unordered_map>

struct IconUV {
    ImVec2 UV0 = { 0, 0 };
    ImVec2 UV1 = { 0, 0 };
    float Scale = 0;
    bool Valid = false;
};

class IconAtlas {
public:
    bool Initialize(void* d3dDevice);
    void Release();

    const IconUV& GetIcon(const std::string& name) const;
    ImTextureID GetTextureID() const { return m_TextureID; }
    bool IsLoaded() const { return m_TextureID != ImTextureID{}; }

private:
    ImTextureID m_TextureID = ImTextureID{};
    ID3D11ShaderResourceView* m_SRV = nullptr;
    int m_TexWidth = 0;
    int m_TexHeight = 0;
    static constexpr float CellSize = 64.0f;

    std::unordered_map<std::string, IconUV> m_Icons;
    IconUV m_EmptyIcon = {};

    void ComputeUV(int gridX, int gridY, float scale, IconUV& out);
    void LoadIconPositions();
    void SetupFallbackIcons();
};
