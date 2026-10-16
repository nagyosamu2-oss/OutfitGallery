#include "Capture.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <ScreenGrab.h>
#include <WICTextureLoader.h>
#include <map>
#include <chrono>
#include <stdexcept>

namespace Gallery {
namespace {
Microsoft::WRL::ComPtr<ID3D11Device> AcquireGalleryDevice() {
    // Renderer::GetDevice is borrowed. ComPtr acquires our own reference.
    // Avoid IDXGISwapChain::GetDevice: the CS D3D12 proxy in the tested
    // generation returns its D3D11 device without the required AddRef.
    // Releasing that result can prematurely destroy ReShade's wrapper state.
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    device = reinterpret_cast<ID3D11Device*>(RE::BSGraphics::Renderer::GetDevice());
    return device;
}
}
LayoutSurface ReadLayoutSurface() {
    LayoutSurface result{};
    auto* window=RE::BSGraphics::Renderer::GetCurrentRenderWindow();
    if(!window || !window->swapChain) return result;
    auto* swap=reinterpret_cast<IDXGISwapChain*>(window->swapChain);
    DXGI_SWAP_CHAIN_DESC desc{};
    if(FAILED(swap->GetDesc(&desc))) return result;
    result.swap={float(desc.BufferDesc.Width),float(desc.BufferDesc.Height)};
    Microsoft::WRL::ComPtr<ID3D11Texture2D> buffer;
    if(FAILED(swap->GetBuffer(0,IID_PPV_ARGS(&buffer)))) return result;
    D3D11_TEXTURE2D_DESC texture{}; buffer->GetDesc(&texture);
    result.buffer={float(texture.Width),float(texture.Height)};
    auto device=AcquireGalleryDevice();
    if(!device) return result;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(&context);
    UINT count=D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT views[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
    context->RSGetViewports(&count,views);
    if(count!=1 || views[0].TopLeftX!=0 || views[0].TopLeftY!=0) return result;
    result.viewport={views[0].Width,views[0].Height};
    result.valid=true;
    return result;
}

namespace {
std::map<std::string,Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> portraitViews;
struct PortraitRetry { std::chrono::steady_clock::time_point next{}; unsigned reports{}; };
std::map<std::string,PortraitRetry> portraitRetries;
std::string LogPath(const std::filesystem::path& path) {
    const auto text=path.generic_u8string();
    return {reinterpret_cast<const char*>(text.data()),text.size()};
}
Microsoft::WRL::ComPtr<ID3D11Texture2D> liveTexture;
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> liveView;
float liveAspect=1.f;
void Check(HRESULT result, const char* operation) {
    if (FAILED(result)) throw std::runtime_error(std::format("{} failed (0x{:08X})", operation, static_cast<unsigned>(result)));
}
// WIC downsamples the cropped image; originals already on disk are never touched.
void ScalePNG(const std::filesystem::path& input, const std::filesystem::path& output, UINT width, UINT height) {
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    Check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)),"WIC factory");
    ComPtr<IWICBitmapDecoder> decoder;
    Check(factory->CreateDecoderFromFilename(input.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder),"Decode portrait");
    ComPtr<IWICBitmapFrameDecode> source;
    Check(decoder->GetFrame(0,&source),"Portrait frame");
    ComPtr<IWICBitmapScaler> scaler;
    Check(factory->CreateBitmapScaler(&scaler),"Create scaler");
    Check(scaler->Initialize(source.Get(),width,height,WICBitmapInterpolationModeFant),"Scale portrait");
    ComPtr<IWICStream> stream;
    Check(factory->CreateStream(&stream),"Create PNG stream");
    Check(stream->InitializeFromFilename(output.c_str(),GENERIC_WRITE),"Open scaled PNG");
    ComPtr<IWICBitmapEncoder> encoder;
    Check(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder),"Create PNG encoder");
    Check(encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache),"Initialize PNG encoder");
    ComPtr<IWICBitmapFrameEncode> frame;
    Check(encoder->CreateNewFrame(&frame,nullptr),"Create PNG frame");
    Check(frame->Initialize(nullptr),"Initialize PNG frame");
    Check(frame->SetSize(width,height),"Set PNG size");
    WICPixelFormatGUID format=GUID_WICPixelFormat24bppBGR;
    Check(frame->SetPixelFormat(&format),"Set PNG format");
    Check(frame->WriteSource(scaler.Get(),nullptr),"Write scaled PNG");
    Check(frame->Commit(),"Commit PNG frame");
    Check(encoder->Commit(),"Commit PNG");
}
}
void CreatePortraitView(ID3D11Device* device,const std::filesystem::path& path,ID3D11ShaderResourceView** view) {
    const HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(com) && com!=RPC_E_CHANGED_MODE) Check(com,"Portrait COM");
    // The standard ImGui shader does not re-encode linear samples for an UNORM
    // output. An SRGB view would decode PNG values and darken the displayed photo.
    const auto result=DirectX::CreateWICTextureFromFileEx(device,path.c_str(),0,D3D11_USAGE_IMMUTABLE,D3D11_BIND_SHADER_RESOURCE,0,0,DirectX::WIC_LOADER_IGNORE_SRGB,nullptr,view);
    if(SUCCEEDED(com)) CoUninitialize();
    Check(result,"Load portrait display texture");
}
void* LoadPortraitView(const std::string& path) {
    auto found=portraitViews.find(path);
    if(found!=portraitViews.end()) return found->second.Get();
    auto& retry=portraitRetries[path];
    const auto now=std::chrono::steady_clock::now();
    if(now<retry.next) return nullptr;
    retry.next=now+std::chrono::seconds(2);
    const bool report=retry.reports<3;
    if(report) ++retry.reports;
    std::error_code absoluteError,existsError,sizeError;
    const auto file=std::filesystem::path(path);
    const auto absolute=std::filesystem::absolute(file,absoluteError);
    const bool exists=std::filesystem::exists(file,existsError);
    const auto bytes=std::filesystem::file_size(file,sizeError);
    if(report) SKSE::log::info("PHOTO-FIX1 load path={} absolute={} exists={} bytes={} pathError={} existsError={} sizeError={}",
        path,absoluteError?"unavailable":LogPath(absolute),exists,sizeError?0:bytes,absoluteError.value(),existsError.value(),sizeError.value());
    auto* window=RE::BSGraphics::Renderer::GetCurrentRenderWindow();
    auto device=AcquireGalleryDevice();
    const bool fallback=false;
    if(report) SKSE::log::info("CS-COMPAT1 portrait device source=renderer-owned-reference window={} swap={} available={}",
        window!=nullptr,window && window->swapChain,bool(device));
    if(!device) return nullptr;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
    try {CreatePortraitView(device.Get(),std::filesystem::path(path),view.GetAddressOf());}
    catch(const std::exception& e){if(report) SKSE::log::warn("PHOTO-FIX1 load failed: {}: {}",path,e.what()); return nullptr;}
    if(!view) {if(report) SKSE::log::warn("PHOTO-FIX1 empty display view: {}",path);return nullptr;}
    SKSE::log::info("PHOTO-FIX1 load success: {} fallback={}",path,fallback);
    portraitRetries.erase(path);
    auto* result=view.Get(); portraitViews.emplace(path,std::move(view)); return result;
}
void DisposePortraitView(const std::string& path) {portraitViews.erase(path);portraitRetries.erase(path);}
void ResetLivePreview() { liveView.Reset(); liveTexture.Reset(); }
void* LivePreviewView() { return liveView.Get(); }
float LivePreviewAspect() { return liveAspect; }
std::array<unsigned,2> LivePreviewSize() {
    D3D11_TEXTURE2D_DESC desc{};
    if(liveTexture) liveTexture->GetDesc(&desc);
    return {desc.Width,desc.Height};
}
void UpdateLivePreview() {
    using Microsoft::WRL::ComPtr;
    auto* window=RE::BSGraphics::Renderer::GetCurrentRenderWindow();
    if(!window || !window->swapChain) throw std::runtime_error("Preview swap chain unavailable");
    auto* swap=reinterpret_cast<IDXGISwapChain*>(window->swapChain);
    ComPtr<ID3D11Texture2D> frame;
    Check(swap->GetBuffer(0,IID_PPV_ARGS(frame.GetAddressOf())),"Preview GetBuffer");
    UpdateLivePreviewTexture(frame.Get());
}
void UpdateLivePreviewTexture(ID3D11Texture2D* frame) {
    using Microsoft::WRL::ComPtr;
    if(!frame) throw std::runtime_error("No preview texture");
    D3D11_TEXTURE2D_DESC source{}; frame->GetDesc(&source);
    if(source.Width<64 || source.Height<64 || source.SampleDesc.Count!=1)
        throw std::runtime_error("Unsupported preview render size or MSAA");
    // Match SavePortraitTexture's centered crop, including integer rounding.
    D3D11_BOX box{UINT(source.Width*.275f),UINT(source.Height*.08f),0,
        UINT(source.Width*.725f),UINT(source.Height*.92f),1};
    auto desc=source;
    desc.Width=box.right-box.left; desc.Height=box.bottom-box.top;
    // Preserve display-referred values just like the saved PNG preview.
    if(desc.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    if(desc.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    if(desc.Format==DXGI_FORMAT_R8G8B8A8_TYPELESS) desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    if(desc.Format==DXGI_FORMAT_B8G8R8A8_TYPELESS) desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    if(desc.Format==DXGI_FORMAT_R10G10B10A2_TYPELESS) desc.Format=DXGI_FORMAT_R10G10B10A2_UNORM;
    if(desc.Format==DXGI_FORMAT_R16G16B16A16_TYPELESS) desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
    if(desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM && desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM &&
       desc.Format!=DXGI_FORMAT_R10G10B10A2_UNORM && desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT &&
       desc.Format!=DXGI_FORMAT_B8G8R8X8_UNORM)
        throw std::runtime_error(std::format("Unsupported preview DXGI format {}",unsigned(source.Format)));
    desc.MipLevels=1; desc.ArraySize=1; desc.Usage=D3D11_USAGE_DEFAULT;
    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE; desc.CPUAccessFlags=0; desc.MiscFlags=0;
    ComPtr<ID3D11Device> device; frame->GetDevice(device.GetAddressOf());
    D3D11_TEXTURE2D_DESC old{};
    ComPtr<ID3D11Device> oldDevice;
    if(liveTexture) {liveTexture->GetDesc(&old); liveTexture->GetDevice(oldDevice.GetAddressOf());}
    if(!liveTexture || old.Width!=desc.Width || old.Height!=desc.Height || old.Format!=desc.Format || device.Get()!=oldDevice.Get()) {
        SKSE::log::info("Live preview: source {}x{}, DXGI {}, view DXGI {}",source.Width,source.Height,unsigned(source.Format),unsigned(desc.Format));
        ResetLivePreview();
        Check(device->CreateTexture2D(&desc,nullptr,liveTexture.GetAddressOf()),"Create live preview");
        Check(device->CreateShaderResourceView(liveTexture.Get(),nullptr,liveView.GetAddressOf()),"Create live preview view");
    }
    ComPtr<ID3D11DeviceContext> context; device->GetImmediateContext(context.GetAddressOf());
    context->CopySubresourceRegion(liveTexture.Get(),0,0,0,0,frame,0,&box);
    liveAspect=float(desc.Width)/float(desc.Height);
}
std::filesystem::path CapturePortrait(bool centered, bool draft) {
    using Microsoft::WRL::ComPtr;
    auto* window = RE::BSGraphics::Renderer::GetCurrentRenderWindow();
    if (!window || !window->swapChain) throw std::runtime_error("Game swap chain is unavailable");
    auto* swap = reinterpret_cast<IDXGISwapChain*>(window->swapChain);
    ComPtr<ID3D11Texture2D> frame;
    Check(swap->GetBuffer(0, IID_PPV_ARGS(frame.GetAddressOf())), "GetBuffer");
    D3D11_TEXTURE2D_DESC captureDesc{};
    frame->GetDesc(&captureDesc);
    SKSE::log::info("Portrait capture: framework pre-render, {}x{}, DXGI format {}, samples {}",captureDesc.Width,captureDesc.Height,static_cast<unsigned>(captureDesc.Format),captureDesc.SampleDesc.Count);
    const auto path=SavePortraitTexture(frame.Get(), draft ? "Data/SKSE/Plugins/OutfitGallery/Captures/PhotoRetake" : "Data/SKSE/Plugins/OutfitGallery/Captures",1024,centered);
    std::error_code pathError,sizeError;
    const auto absolute=std::filesystem::absolute(path,pathError);
    const auto bytes=std::filesystem::file_size(path,sizeError);
    SKSE::log::info("PHOTO-FIX1 saved path={} absolute={} bytes={} pathError={} sizeError={}",
        LogPath(path),pathError?"unavailable":LogPath(absolute),sizeError?0:bytes,pathError.value(),sizeError.value());
    return path;
}
std::filesystem::path SavePortraitTexture(ID3D11Texture2D* frame, const std::filesystem::path& folder, unsigned maxWidth, bool centered) {
    using Microsoft::WRL::ComPtr;
    if (!frame) throw std::runtime_error("No texture to capture");
    if(!maxWidth) throw std::runtime_error("Invalid portrait width");
    D3D11_TEXTURE2D_DESC desc{};
    frame->GetDesc(&desc);
    if (desc.Width < 64 || desc.Height < 64 || desc.SampleDesc.Count != 1) throw std::runtime_error("Unsupported render size or MSAA backbuffer");
    // Same normalized rectangle as the on-screen portrait guide.
    D3D11_BOX box{ static_cast<UINT>(desc.Width * (centered ? 0.275f : 0.54f)), static_cast<UINT>(desc.Height * 0.08f), 0,
        static_cast<UINT>(desc.Width * (centered ? 0.725f : 0.99f)), static_cast<UINT>(desc.Height * 0.92f), 1 };
    ComPtr<ID3D11Device> device;
    frame->GetDevice(device.GetAddressOf());
    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());
    auto cropDesc = desc;
    cropDesc.Width = box.right - box.left;
    cropDesc.Height = box.bottom - box.top;
    cropDesc.MipLevels = 1;
    cropDesc.ArraySize = 1;
    cropDesc.Usage = D3D11_USAGE_DEFAULT;
    cropDesc.BindFlags = 0;
    cropDesc.CPUAccessFlags = 0;
    cropDesc.MiscFlags = 0;
    ComPtr<ID3D11Texture2D> crop;
    Check(device->CreateTexture2D(&cropDesc, nullptr, crop.GetAddressOf()), "Create crop");
    context->CopySubresourceRegion(crop.Get(), 0, 0, 0, 0, frame, 0, &box);

    std::filesystem::create_directories(folder);
    const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto output = folder / std::format("portrait-{}.png", stamp);
    const auto temporary = folder / std::format("portrait-{}.tmp.png", stamp);
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) Check(com, "COM initialization");
    auto scaled=temporary; scaled+=".scaled.png";
    try {
        Check(DirectX::SaveWICTextureToFile(context.Get(), crop.Get(), GUID_ContainerFormatPng, temporary.c_str()),"PNG encoding");
        if(cropDesc.Width>maxWidth) {
            const auto height=std::max(1u,static_cast<UINT>(std::lround(double(cropDesc.Height)*maxWidth/cropDesc.Width)));
            ScalePNG(temporary,scaled,maxWidth,height);
            std::filesystem::rename(scaled,output);
            std::error_code ignored; std::filesystem::remove(temporary,ignored);
        } else std::filesystem::rename(temporary,output);
    } catch(...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        std::filesystem::remove(scaled, ignored);
        if(SUCCEEDED(com)) CoUninitialize();
        throw;
    }
    if(SUCCEEDED(com)) CoUninitialize();
    return output;
}
}
