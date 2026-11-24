// Scarlett Fusion Lab archive: generic, original Windows D3D12/DXGI feature probe.
// Uses public Windows SDK APIs only; no third-party game assets or console code.
// MIT-licensed probe source; see LICENSE.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

using Microsoft::WRL::ComPtr;

static std::string utf8(const wchar_t* value) {
    if (value == nullptr) return {};
    const int length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (length < 1) return {};
    std::vector<char> buffer(static_cast<size_t>(length));
    if (!WideCharToMultiByte(CP_UTF8, 0, value, -1, buffer.data(), length, nullptr, nullptr)) {
        return {};
    }
    return std::string(buffer.data());
}

static std::string jsonEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (unsigned char ch : input) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) {
                    char hex[7] = {};
                    std::snprintf(hex, sizeof(hex), "\\u%04x", static_cast<unsigned>(ch));
                    out += hex;
                } else {
                    out += static_cast<char>(ch);
                }
        }
    }
    return out;
}

static void printBoolean(bool value) {
    std::cout << (value ? "true" : "false");
}


// Render an HRESULT as eight-digit hexadecimal for diagnostic purposes.
static std::string hexHr(HRESULT hr) {
    char buffer[16] = {};
    std::snprintf(buffer, sizeof(buffer), "0x%08X", static_cast<unsigned>(hr));
    return buffer;
}

// A lightweight DXGI-only snapshot: no D3D12 device or GPU allocations.
// CurrentUsage always refers to THIS probe process, not other applications.
static void printBudgetGroup(IDXGIAdapter3* adapter, DXGI_MEMORY_SEGMENT_GROUP group) {
    DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
    const HRESULT hr = adapter ? adapter->QueryVideoMemoryInfo(0, group, &info) : E_NOINTERFACE;
    std::cout << "{\"query_hr\":\"" << hexHr(hr) << "\",\"ok\":";
    printBoolean(SUCCEEDED(hr));
    if (SUCCEEDED(hr)) {
        std::cout << ",\"budget_bytes\":" << static_cast<unsigned long long>(info.Budget)
                  << ",\"current_usage_bytes\":" << static_cast<unsigned long long>(info.CurrentUsage)
                  << ",\"available_for_reservation_bytes\":" << static_cast<unsigned long long>(info.AvailableForReservation)
                  << ",\"current_reservation_bytes\":" << static_cast<unsigned long long>(info.CurrentReservation);
    } else {
        std::cout << ",\"budget_bytes\":null,\"current_usage_bytes\":null"
                  << ",\"available_for_reservation_bytes\":null,\"current_reservation_bytes\":null";
    }
    std::cout << "}";
}

static int printBudgetOnlySnapshot(IDXGIFactory1* factory) {
    std::cout << "{\n  \"schema_version\": 1,\n"
              << "  \"kind\":\"dxgi-probe-process-budget\",\n"
              << "  \"usage_scope\":\"probe-process-only\",\n"
              << "  \"adapters\":[";
    bool first = true;
    for (UINT index = 0; ; ++index) {
        ComPtr<IDXGIAdapter1> adapter;
        const HRESULT hr = factory->EnumAdapters1(index, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(hr)) {
            std::cerr << "DXGI adapter enumeration failed at index " << index
                      << ": " << hexHr(hr) << "\n";
            return 3;
        }
        DXGI_ADAPTER_DESC1 desc = {};
        if (FAILED(adapter->GetDesc1(&desc))) continue;
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) continue;
        ComPtr<IDXGIAdapter3> adapter3;
        const HRESULT queryInterfaceHr = adapter.As(&adapter3);
        if (!first) std::cout << ",";
        first = false;
        std::cout << "\n    {\"index\":" << index
                  << ",\"name\":\"" << jsonEscape(utf8(desc.Description)) << "\""
                  << ",\"vendor_id\":" << desc.VendorId
                  << ",\"device_id\":" << desc.DeviceId
                  << ",\"luid_high\":" << desc.AdapterLuid.HighPart
                  << ",\"luid_low\":" << desc.AdapterLuid.LowPart
                  << ",\"dedicated_video_memory_bytes\":"
                  << static_cast<unsigned long long>(desc.DedicatedVideoMemory)
                  << ",\"shared_system_memory_bytes\":"
                  << static_cast<unsigned long long>(desc.SharedSystemMemory)
                  << ",\"adapter3_hr\":\"" << hexHr(queryInterfaceHr) << "\""
                  << ",\"local\":";
        printBudgetGroup(adapter3.Get(), DXGI_MEMORY_SEGMENT_GROUP_LOCAL);
        std::cout << ",\"non_local\":";
        printBudgetGroup(adapter3.Get(), DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL);
        std::cout << "}";
    }
    std::cout << "\n  ],\n"
              << "  \"note\":\"DXGI CurrentUsage and CurrentReservation belong to this short-lived probe process only. The reported Budget is an estimate for the probe, not other applications.\"\n"
              << "}\n";
    return 0;
}

int main(int argc, char** argv) {
    const bool budgetsOnly = argc == 2 && std::strcmp(argv[1], "--budgets-only") == 0;
    if (argc > 1 && !budgetsOnly) {
        std::cerr << "Usage: d3d12_caps.exe [--budgets-only]\n";
        return 64;
    }
    ComPtr<IDXGIFactory1> factory;
    const HRESULT factoryHr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(factoryHr)) {
        std::cerr << "CreateDXGIFactory1 failed: 0x" << std::hex
                  << static_cast<unsigned>(factoryHr) << "\n";
        return 2;
    }

    if (budgetsOnly) return printBudgetOnlySnapshot(factory.Get());

    std::cout << "{\n  \"schema_version\": 1,\n  \"adapters\": [";
    bool first = true;
    for (UINT index = 0; ; ++index) {
        ComPtr<IDXGIAdapter1> adapter;
        const HRESULT enumHr = factory->EnumAdapters1(index, &adapter);
        if (enumHr == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(enumHr)) {
            std::cerr << "EnumAdapters1 failed at index " << index << "\n";
            break;
        }
        DXGI_ADAPTER_DESC1 desc = {};
        if (FAILED(adapter->GetDesc1(&desc))) continue;
        if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) continue;

        ComPtr<ID3D12Device> device;
        const bool featureLevel12 =
            SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
        bool created = featureLevel12;
        if (!created) {
            device.Reset();
            created = SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
        }

        bool shaderModelMeasured = false;
        unsigned shaderModel = 0;
        bool meshTierMeasured = false;
        unsigned meshTier = 0;
        bool meshSupported = false;
        bool waveMeasured = false;
        UINT waveMin = 0, waveMax = 0;

        if (created) {
            // Newer shader-model enum values are rejected by older D3D12 runtimes.
            // Fall back to an older query rather than misreporting SM 6.6.
            const D3D_SHADER_MODEL candidates[] = {
                D3D_SHADER_MODEL_6_6,
                D3D_SHADER_MODEL_6_5,
                D3D_SHADER_MODEL_6_0
            };
            for (D3D_SHADER_MODEL requested : candidates) {
                D3D12_FEATURE_DATA_SHADER_MODEL data = {requested};
                if (SUCCEEDED(device->CheckFeatureSupport(
                        D3D12_FEATURE_SHADER_MODEL, &data, sizeof(data)))) {
                    shaderModel = static_cast<unsigned>(data.HighestShaderModel);
                    shaderModelMeasured = true;
                    break;
                }
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
            meshTierMeasured = SUCCEEDED(device->CheckFeatureSupport(
                D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)));
            if (meshTierMeasured) {
                meshTier = static_cast<unsigned>(options7.MeshShaderTier);
                meshSupported = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
            }

            D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1 = {};
            waveMeasured = SUCCEEDED(device->CheckFeatureSupport(
                D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1)));
            if (waveMeasured) {
                waveMin = options1.WaveLaneCountMin;
                waveMax = options1.WaveLaneCountMax;
            }
        }

        if (!first) std::cout << ",";
        first = false;
        std::cout << "\n    {\n"
                  << "      \"name\": \"" << jsonEscape(utf8(desc.Description)) << "\",\n"
                  << "      \"vendor_id\": " << desc.VendorId << ",\n"
                  << "      \"device_id\": " << desc.DeviceId << ",\n"
                  << "      \"dedicated_video_memory_mb\": "
                  << static_cast<unsigned long long>(desc.DedicatedVideoMemory / (1024 * 1024)) << ",\n"
                  << "      \"d3d12_device\": ";
        printBoolean(created);
        std::cout << ",\n      \"feature_level_12_0\": ";
        printBoolean(featureLevel12);
        std::cout << ",\n      \"shader_model\": ";
        if (shaderModelMeasured) {
            std::cout << "\"" << ((shaderModel >> 4) & 15) << "." << (shaderModel & 15) << "\"";
        } else {
            std::cout << "null";
        }
        std::cout << ",\n      \"mesh_shader_tier\": ";
        if (meshTierMeasured) std::cout << meshTier; else std::cout << "null";
        std::cout << ",\n      \"mesh_shader_supported\": ";
        if (meshTierMeasured) printBoolean(meshSupported); else std::cout << "null";
        std::cout << ",\n      \"wave_lane_min\": ";
        if (waveMeasured) std::cout << waveMin; else std::cout << "null";
        std::cout << ",\n      \"wave_lane_max\": ";
        if (waveMeasured) std::cout << waveMax; else std::cout << "null";
        std::cout << "\n    }";
    }
    std::cout << "\n  ],\n"
              << "  \"note\": \"Generic Windows GPU feature query only. No third-party software, external files, or game content accessed.\"\n"
              << "}\n";
    return 0;
}
