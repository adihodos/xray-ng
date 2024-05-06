#include "gpu_shader_common_core.hpp"
#include "scoped_resource_mapping.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/debug/debug_ext.hpp"
#include "xray/base/shims/string.hpp"
#include <algorithm>
#include <d3d10shader.h>
#include <d3dcompiler.h>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <stlsoft/memory/auto_buffer.hpp>

using namespace xray::base;

xray::rendering::directx10::scoped_blob_handle
xray::rendering::directx10::compile_shader(const shader_type type,
                                           const char* src_code,
                                           const size_t code_len,
                                           const uint32_t compile_flags)
{

    assert(src_code != nullptr);

    constexpr const char* targets[] = { "vs_4_0", "gs_4_0", "ps_4_0" };
    assert((uint32_t)type < XR_U32_COUNTOF__(targets));

    scoped_blob_handle err_msg_blob;
    scoped_blob_handle bytecode_blob;
    D3DCompile(src_code,
               code_len,
               nullptr,
               nullptr,
               nullptr,
               "main",
               targets[(uint32_t)type],
               compile_flags,
               0,
               raw_ptr_ptr(bytecode_blob),
               raw_ptr_ptr(err_msg_blob));

    if (!bytecode_blob) {
        XR_LOG_DEBUG("Failed to compile shader!");
        auto err_msg = static_cast<const char*>(err_msg_blob->GetBufferPointer());
        if (err_msg) {
            XR_LOG_DEBUG("Error %s", err_msg);
        }
    }

    return bytecode_blob;
}

xray::rendering::directx10::scoped_blob_handle
xray::rendering::directx10::compile_shader_from_file(const shader_type type,
                                                     const char* file_name,
                                                     const uint32_t compile_flags)
{

    assert(file_name != nullptr);

    try {
        platformstl::memory_mapped_file mm_file{ file_name };
        return compile_shader(type, static_cast<const char*>(mm_file.memory()), mm_file.size(), compile_flags);
    } catch (const std::exception& e) {
        XR_LOG_DEBUG("Failed to open shader file %s", file_name);
    }

    return {};
}

bool
xray::rendering::directx10::detail::shader_collect_uniforms(ID3D10Blob* compiled_bytecode,
                                                            std::vector<uniform_t>& uniforms,
                                                            std::vector<uniform_block_t>& uniform_blocks,
                                                            uint32_t& storage_bytes)
{
    using namespace xray::base;
    using namespace std;

    com_ptr<ID3D10ShaderReflection> reflector;
    const auto ret_code = D3D10ReflectShader(
        compiled_bytecode->GetBufferPointer(), compiled_bytecode->GetBufferSize(), raw_ptr_ptr(reflector));

    if (ret_code != S_OK) {
        XR_LOG_DEBUG("Failed to reflect shader!, %#08x", ret_code);
        return false;
    }

    D3D10_SHADER_DESC shader_desc;
    if (reflector->GetDesc(&shader_desc) != S_OK) {
        XR_LOG_DEBUG("Failed to get shader description !");
        return false;
    }

    struct ublock_reflect_data
    {
        uniform_block_t ublock;
        ID3D10ShaderReflectionConstantBuffer* reflector;
        uint32_t variables{};
    };

    std::vector<ublock_reflect_data> ublocks;
    uint32_t ublock_store_off{};

    for (uint32_t idx = 0; idx < shader_desc.BoundResources; ++idx) {
        D3D10_SHADER_INPUT_BIND_DESC rbd;
        if (reflector->GetResourceBindingDesc(idx, &rbd) != S_OK) {
            XR_LOG_DEBUG("Failed to get binding info for resource %u", idx);
            return false;
        }

        XR_LOG_DEBUG("resource %s, bind point %u, bind count %u", rbd.Name, rbd.BindPoint, rbd.BindCount);

        if (rbd.Type == D3D10_SIT_CBUFFER) {
            //
            // reflect constant buffer
            auto cbuff_ref = reflector->GetConstantBufferByName(rbd.Name);
            if (!cbuff_ref) {
                XR_LOG_DEBUG("Failed to reflect constant buffer %s", rbd.Name);
                return false;
            }

            D3D10_SHADER_BUFFER_DESC cbuff_desc;
            if (cbuff_ref->GetDesc(&cbuff_desc) != S_OK) {
                XR_LOG_DEBUG("Failed to get description for cbuffer %s", rbd.Name);
                return false;
            }

            ublocks.push_back(
                { { rbd.Name, rbd.BindPoint, cbuff_desc.Size, ublock_store_off }, cbuff_ref, cbuff_desc.Variables });

            // uniform_block_t{rbd.Name, rbd.BindPoint,
            //                                 cbuff_desc.Size, ublock_store_off});
            ublock_store_off += cbuff_desc.Size;

        } else {
            continue;
        }
    }

    sort(begin(ublocks), end(ublocks), [](const auto& b0, const auto& b1) {
        return b0.ublock.bind_point < b1.ublock.bind_point;
    });

    vector<uniform_t> standalone_uniforms;
    for (size_t idx = 0, ublock_count = ublocks.size(); idx < ublock_count; ++idx) {
        const auto& ublk = ublocks[idx];

        for (uint32_t var_idx = 0; var_idx < ublk.variables; ++var_idx) {
            auto var_reflect = ublk.reflector->GetVariableByIndex(var_idx);
            if (!var_reflect) {
                XR_LOG_DEBUG("Failed to reflect variable with index %u", var_idx);
                return false;
            }

            // struct uniform_t {
            //   std::string name{};
            //   uint32_t    parent_idx{0};
            //   uint32_t    store_offset{0};
            //   uint32_t    byte_size{0};
            // };
            D3D10_SHADER_VARIABLE_DESC var_desc;
            if (var_reflect->GetDesc(&var_desc) != S_OK) {
                XR_LOG_DEBUG("Failed to get description for variable with index %u", var_idx);
                return false;
            }

            standalone_uniforms.push_back(
                { var_desc.Name, static_cast<uint32_t>(idx), var_desc.StartOffset, var_desc.Size });
        }
    }

    // struct uniform_block_t {
    //   std::string name{};
    //   uint32_t    bind_point{0};
    //   uint32_t    byte_size{0};
    //   uint32_t    store_offset{0};
    // };

    for_each(begin(ublocks), end(ublocks), [](const auto& blk) {
        XR_LOG_DEBUG("Uniform block %s, bind point %u, byte size %u, store offset %u",
                       raw_str(blk.ublock.name),
                       blk.ublock.bind_point,
                       blk.ublock.byte_size,
                       blk.ublock.store_offset);
    });

    for_each(begin(standalone_uniforms), end(standalone_uniforms), [](const auto& uf) {
        XR_LOG_DEBUG("Uniform %s, parent index %u, offset %u, byte size %u",
                       raw_str(uf.name),
                       uf.parent_idx,
                       uf.store_offset,
                       uf.byte_size);
    });

    transform(begin(ublocks), end(ublocks), back_inserter(uniform_blocks), [](const auto& ublk_data) {
        return ublk_data.ublock;
    });

    uniforms = move(standalone_uniforms);
    storage_bytes = ublock_store_off;

    return true;
}

void
xray::rendering::directx10::gpu_shader_common_core::set_uniform(const char* name,
                                                                const void* value,
                                                                const size_t bytes) noexcept
{
    assert(name != nullptr);
    assert(value != nullptr);

    assert(valid());

    auto uf_iterator =
        find_if(begin(uniforms_), end(uniforms_), [name](const auto& u_data) { return name == u_data.name; });

    if (uf_iterator == end(uniforms_)) {
        XR_LOG_DEBUG("Trying to set non-existant uniform %s", name);
        return;
    }

    auto parent_block = &uniform_blocks_[uf_iterator->parent_idx];
    const auto dst_offset = parent_block->store_offset + uf_iterator->store_offset;

    assert((dst_offset + static_cast<uint32_t>(bytes)) <= parent_block->byte_size);

    auto mem_dst = raw_ptr(ublocks_storage_) + dst_offset;
    memcpy(mem_dst, value, bytes);
    parent_block->dirty = true;
}

void
xray::rendering::directx10::gpu_shader_common_core::set_uniform_block(const char* name,
                                                                      const void* value,
                                                                      const size_t bytes) noexcept
{

    assert(name != nullptr);
    assert(value != nullptr);
    assert(valid());

    auto ublock_iterator =
        find_if(begin(uniform_blocks_), end(uniform_blocks_), [name](const auto& ublk) { return ublk.name == name; });

    if (ublock_iterator == end(uniform_blocks_)) {
        XR_LOG_DEBUG("Uniform block %s does not exist", name);
        return;
    }

    assert(static_cast<uint32_t>(bytes) <= ublock_iterator->byte_size);
    auto dst_mem = raw_ptr(ublocks_storage_) + ublock_iterator->store_offset;
    memcpy(dst_mem, value, bytes);
    ublock_iterator->dirty = true;
}

xray::rendering::directx10::gpu_shader_common_core::gpu_shader_common_core(const shader_type stype,
                                                                           ID3D10Device* dev,
                                                                           const char* file_name,
                                                                           const uint32_t compile_flags)
{

    if ((bytecode_ = compile_shader_from_file(stype, file_name, compile_flags)))
        collect_uniforms(dev);
}

xray::rendering::directx10::gpu_shader_common_core::gpu_shader_common_core(const shader_type stype,
                                                                           ID3D10Device* dev,
                                                                           const char* src_code,
                                                                           const size_t code_len,
                                                                           const uint32_t compile_flags)
{

    if ((bytecode_ = compile_shader(stype, src_code, code_len, compile_flags)))
        collect_uniforms(dev);
}

xray::rendering::directx10::gpu_shader_common_core::~gpu_shader_common_core() {}

void
xray::rendering::directx10::gpu_shader_common_core::collect_uniforms(ID3D10Device* dev)
{

    assert(bytecode_ != nullptr);

    uint32_t bytes_storage{};
    if (!detail::shader_collect_uniforms(raw_ptr(bytecode_), uniforms_, uniform_blocks_, bytes_storage)) {
        return;
    }

    ublocks_storage_ = base::unique_pointer<uint8_t[]>(new uint8_t[bytes_storage]);

    using namespace std;
    transform(begin(uniform_blocks_), end(uniform_blocks_), back_inserter(cbuffers_), [dev](const auto& ublock) {
        D3D10_BUFFER_DESC buff_desc;
        buff_desc.ByteWidth = ublock.byte_size;
        buff_desc.Usage = D3D10_USAGE_DYNAMIC;
        buff_desc.BindFlags = D3D10_BIND_CONSTANT_BUFFER;
        buff_desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
        buff_desc.MiscFlags = 0;

        com_ptr<ID3D10Buffer> cbuffer;
        dev->CreateBuffer(&buff_desc, nullptr, raw_ptr_ptr(cbuffer));
        return cbuffer;
    });

    const auto failed = any_of(begin(cbuffers_), end(cbuffers_), [](const auto& cbuff) { return cbuff == nullptr; });

    if (failed) {
        XR_LOG_DEBUG("Failed to create some constant buffers !");
        return;
    }

    valid_ = true;
}

void
xray::rendering::directx10::gpu_shader_common_core::bind(
    ID3D10Device* dev,
    void (*stage_bind_fn)(ID3D10Device*, const uint32_t, const uint32_t, ID3D10Buffer* const*))
{
    assert(valid());

    if (uniform_blocks_.empty())
        return;

    using namespace std;

    //
    // flush uniforms to GPU
    {
        for (size_t idx = 0, block_cnt = uniform_blocks_.size(); idx < block_cnt; ++idx) {
            auto& ublock = uniform_blocks_[idx];
            if (!ublock.dirty)
                continue;

            auto& cbuff = cbuffers_[idx];
            scoped_buffer_mapping sbm{ raw_ptr(cbuff), D3D10_MAP_WRITE_DISCARD };
            if (!sbm) {
                XR_LOG_DEBUG("Failed to map constant buffer for updating!");
                continue;
            }

            memcpy(sbm.memory(), raw_ptr(ublocks_storage_) + ublock.store_offset, ublock.byte_size);
            ublock.dirty = false;
        }
    }

    //
    // bind constant buffers
    {
        stlsoft::auto_buffer<ID3D10Buffer*, 32> buffers_to_bind{ cbuffers_.size() };
        transform(
            begin(cbuffers_), end(cbuffers_), begin(buffers_to_bind), [](const auto& cbuff) { return raw_ptr(cbuff); });

        stage_bind_fn(
            dev, uniform_blocks_[0].bind_point, static_cast<uint32_t>(cbuffers_.size()), buffers_to_bind.data());
    }
}
