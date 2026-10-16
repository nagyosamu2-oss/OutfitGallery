#pragma once
#include <array>
#include "ScaleLayout.h"
struct ID3D11Texture2D;
struct ID3D11Device;
struct ID3D11ShaderResourceView;
namespace Gallery {
LayoutSurface ReadLayoutSurface(); // Render-thread, read-only; no GPU copy/readback.
// Display-referred PNG values are passed unchanged to the ImGui shader.
void CreatePortraitView(ID3D11Device* device, const std::filesystem::path& path, ID3D11ShaderResourceView** view);
void* LoadPortraitView(const std::string& path);
void DisposePortraitView(const std::string& path);
// Called only in the framework's pre-render callback, before its ImGui draw.
std::filesystem::path CapturePortrait(bool centered=false, bool draft=false);
std::filesystem::path SavePortraitTexture(ID3D11Texture2D* frame, const std::filesystem::path& folder, unsigned maxWidth=1024, bool centered=false);
// Render-thread only. Copies the backbuffer before framework UI drawing.
void UpdateLivePreview();
void UpdateLivePreviewTexture(ID3D11Texture2D* frame); // same render-thread operation, testable without Skyrim
void ResetLivePreview();
void* LivePreviewView();
float LivePreviewAspect();
std::array<unsigned,2> LivePreviewSize();
}

