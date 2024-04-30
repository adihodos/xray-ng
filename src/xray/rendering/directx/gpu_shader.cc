#include "xray/rendering/directx/gpu_shader.hpp"
#include "xray/base/logger.hpp"
#include <d3dcompiler.h>
#include <platformstl/filesystem/memory_mapped_file.hpp>

using namespace xray::base;

xray::base::com_ptr<ID3D10Blob>
xray::rendering::compile_shader(const char* src_code,
                                const uint32_t code_len,
                                const char* entry_pt,
                                const char* target,
                                const uint32_t compile_flags)
{
    com_ptr<ID3D10Blob> blob_bytecode;
    com_ptr<ID3D10Blob> blob_errmsg;

    D3DCompile(src_code,
               code_len,
               nullptr,
               nullptr,
               D3D_COMPILE_STANDARD_FILE_INCLUDE,
               entry_pt,
               target,
               compile_flags,
               0,
               raw_ptr_ptr(blob_bytecode),
               raw_ptr_ptr(blob_errmsg));

    if (!blob_bytecode && blob_errmsg) {
        XR_LOG_ERR("Shader compilation failed, error {}", static_cast<const char*>(blob_errmsg->GetBufferPointer()));
    }

    return blob_bytecode;
}

xray::base::com_ptr<ID3D10Blob>
xray::rendering::compile_shader(const char* file,
                                const char* entry_pt,
                                const char* target,
                                const uint32_t compile_flags)
{
    try {
        platformstl::memory_mapped_file shader_file{ file };
        return compile_shader(static_cast<const char*>(shader_file.memory()),
                              static_cast<uint32_t>(shader_file.size()),
                              entry_pt,
                              target,
                              compile_flags);
    } catch (const std::exception& e) {
        XR_LOG_ERR("Failed to open shader file {}, error {}", file, e.what());
    }

    return nullptr;
}
