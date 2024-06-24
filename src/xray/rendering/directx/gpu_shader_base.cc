//
// Copyright (c) 2011, 2012, 2013, 2014, 2015, 2016 Adrian Hodos
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the author nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTORS BE LIABLE FOR
// ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "xray/rendering/directx/gpu_shader_base.hpp"
#include "xray/base/fnv_hash.hpp"
#include "xray/base/logger.hpp"
#include <algorithm>
#include <d3dcompiler.h>
#include <numeric>

using namespace xray::base;
using namespace std;

xray::rendering::detail::shader_uniform_block::shader_uniform_block(ID3D11Device* device_context,
                                                                    const D3D11_SHADER_BUFFER_DESC& buff_desc,
                                                                    const uint32_t bind_point)
    : sub_store_offset{ 0 }
    , sub_name{ buff_desc.Name }
    , sub_hashed_name{ FNV::fnv1a(buff_desc.Name) }
    , sub_var_count{ buff_desc.Variables }
    , sub_bytes_size{ buff_desc.Size }
    , sub_gpu_bindpoint{ bind_point }
    , sub_dirty{ false }
{

    const D3D11_BUFFER_DESC buffer_description = {
        buff_desc.Size, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, buff_desc.Size
    };

    const auto ret_code = device_context->CreateBuffer(&buffer_description, nullptr, base::raw_ptr_ptr(sub_gpu_buffer));

    if (ret_code != S_OK)
        XR_LOG_ERR("Failed to create cbuffer !");
}

xray::rendering::shader_common_core::shader_common_core(ID3D11Device* device, ID3D10Blob* compiled_bytecode)
{

    if (!compiled_bytecode)
        return;

    com_ptr<ID3D11ShaderReflection> shader_reflector{};
    {
        const auto ret_code = D3DReflect(compiled_bytecode->GetBufferPointer(),
                                         compiled_bytecode->GetBufferSize(),
                                         __uuidof(ID3D11ShaderReflection),
                                         reinterpret_cast<void**>(raw_ptr_ptr(shader_reflector)));

        if (!shader_reflector) {
            XR_LOG_ERR("Failed to reflect shader, error code {}", ret_code);
            return;
        }
    }

    D3D11_SHADER_DESC shader_description;
    if (shader_reflector->GetDesc(&shader_description) != S_OK) {
        XR_LOG_ERR("Failed to get shader description.");
        return;
    }

    if (shader_description.BoundResources == 0) {
        _valid = true;
        return;
    }

    std::vector<detail::shader_uniform_block> ublocks;
    std::vector<detail::shader_uniform> uniforms;
    std::vector<detail::shader_resource_view> srvs;
    std::vector<detail::sampler_state> samplers;

    for (uint32_t idx = 0; idx < shader_description.BoundResources; ++idx) {
        D3D11_SHADER_INPUT_BIND_DESC input_desc;
        if (shader_reflector->GetResourceBindingDesc(idx, &input_desc) != S_OK) {
            XR_LOG_ERR("Failed to get resource {} binding description", idx);
            return;
        }

        if (input_desc.Type == D3D_SIT_CBUFFER) {
            auto buff_reflector = shader_reflector->GetConstantBufferByName(input_desc.Name);

            if (!buff_reflector) {
                XR_LOG_ERR("Failed to reflect cbuffer {}", input_desc.Name);
                return;
            }

            D3D11_SHADER_BUFFER_DESC buffer_desc;
            if (buff_reflector->GetDesc(&buffer_desc) != S_OK) {
                XR_LOG_ERR("Failed to get description for cbuffer {}", input_desc.Name);
                return;
            }

            auto new_ublock = detail::shader_uniform_block{ device, buffer_desc, input_desc.BindPoint };
            if (!new_ublock) {
                XR_LOG_ERR("Failed to initialze new uniform block");
                return;
            }

            const auto block_hashed_name = new_ublock.sub_hashed_name;
            ublocks.push_back(std::move(new_ublock));

            for (uint32_t var = 0; var < buffer_desc.Variables; ++var) {
                auto var_reflect = buff_reflector->GetVariableByIndex(var);
                if (!var_reflect) {
                    XR_LOG_ERR("Failed to reflect shader cbuffer member variable {}", var);
                    return;
                }

                D3D11_SHADER_VARIABLE_DESC var_desc;
                if (var_reflect->GetDesc(&var_desc) != S_OK) {
                    XR_LOG_ERR("Failed to get shader variable description");
                    return;
                }

                uniforms.push_back({ var_desc, block_hashed_name });
            }

            continue;
        }

        if (input_desc.Type == D3D_SIT_SAMPLER) {
            detail::sampler_state smp_state;
            smp_state.ss_bindpoint = input_desc.BindPoint;
            smp_state.ss_name = input_desc.Name;

            samplers.push_back(smp_state);
            continue;
        }

        if (input_desc.Type == D3D_SIT_TEXTURE) {
            detail::shader_resource_view srv;
            srv.srv_bindcount = input_desc.BindCount;
            srv.srv_bindpoint = input_desc.BindPoint;
            srv.srv_dimension = input_desc.Dimension;
            srv.srv_name = input_desc.Name;
            srvs.push_back(srv);

            continue;
        }
    }

    _uniform_blocks = move(ublocks);
    _uniforms = move(uniforms);

    if (!_uniform_blocks.empty()) {
        sort(begin(_uniform_blocks), end(_uniform_blocks), [](const auto& b0, const auto& b1) {
            return b0.sub_gpu_bindpoint < b1.sub_gpu_bindpoint;
        });

        _st_slots.ss_cbuffer = _uniform_blocks[0].sub_gpu_bindpoint;

        const auto storage_bytes = accumulate(
            begin(_uniform_blocks), end(_uniform_blocks), uint32_t{}, [](const auto bytes, const auto& ublock) {
                return bytes + ublock.sub_bytes_size;
            });

        _datastore = xray::base::make_unique<uint8_t[]>(storage_bytes);
    }

    _resource_views = move(srvs);
    if (!_resource_views.empty()) {
        sort(begin(_resource_views), end(_resource_views), [](const auto& s0, const auto& s1) {
            return s0.srv_bindpoint < s1.srv_bindpoint;
        });

        _st_slots.ss_resourceviews = _resource_views[0].srv_bindpoint;
    }

    _samplers = move(samplers);
    if (!_samplers.empty()) {
        sort(begin(_samplers), end(_samplers), [](const auto& s0, const auto& s1) {
            return s0.ss_bindpoint < s1.ss_bindpoint;
        });

        _st_slots.ss_samplers = _samplers[0].ss_bindpoint;
    }

    _valid = true;
}

xray::rendering::shader_common_core::~shader_common_core() {}

void
xray::rendering::shader_common_core::set_uniform_block(const char* block_name,
                                                       const void* block_data,
                                                       const size_t bytes_count) noexcept
{

    assert(_valid);

    const auto hashed_name = FNV::fnv1a(block_name);
    auto block = find_if(begin(_uniform_blocks), end(_uniform_blocks), [hashed_name](const auto& blk) {
        return hashed_name == blk.sub_hashed_name;
    });

    if (block == std::end(_uniform_blocks)) {
        XR_LOG_ERR("Uniform block {} does not exist !", block_name);
        return;
    }

    assert(bytes_count <= block->sub_bytes_size);

    auto block_mem = raw_ptr(_datastore) + block->sub_store_offset;
    memcpy(block_mem, block_data, bytes_count);
    block->sub_dirty = true;
}

void
xray::rendering::shader_common_core::set_uniform(const char* uniform_name,
                                                 const void* uniform_data,
                                                 const size_t bytes_count) noexcept
{

    assert(_valid);

    const auto hashed_name = FNV::fnv1a(uniform_name);

    auto uniform_itr = find_if(
        begin(_uniforms), end(_uniforms), [hashed_name](const auto& uf) { return hashed_name == uf.su_hashed_name; });

    if (uniform_itr == std::end(_uniforms)) {
        XR_LOG_ERR("Uniform {} does not exist !", uniform_name);
        return;
    }

    assert(bytes_count <= uniform_itr->su_bytes_size);

    auto parent_block = find_if(begin(_uniform_blocks),
                                end(_uniform_blocks),
                                [parent_id = uniform_itr->su_parent_block_id](const auto& ublock) {
                                    return parent_id == ublock.sub_hashed_name;
                                });

    assert(parent_block != end(_uniform_blocks));

    auto mem_dst_ptr = raw_ptr(_datastore) + parent_block->sub_store_offset + uniform_itr->su_offset_in_parent;
    memcpy(mem_dst_ptr, uniform_data, bytes_count);
    parent_block->sub_dirty = true;
}

void
xray::rendering::shader_common_core::set_resource_view(const char* rv_name, ID3D11ShaderResourceView* rv_handle)
{

    assert(_valid);
    auto rv_iter = find_if(
        begin(_resource_views), end(_resource_views), [rv_name](const auto& rv) { return rv_name == rv.srv_name; });

    if (rv_iter == end(_resource_views)) {
        XR_LOG_ERR("Resource view {} does not exist !", rv_name);
        return;
    }

    rv_iter->srv_bound_view = rv_handle;
}

void
xray::rendering::shader_common_core::set_sampler(const char* sampler_name, ID3D11SamplerState* sampler_state)
{

    assert(_valid);
    assert(sampler_name != nullptr);

    auto sampler_state_itr = find_if(begin(_samplers), end(_samplers), [sampler_name](const auto& sstate) {
        return sampler_name == sstate.ss_name;
    });

    if (sampler_state_itr == end(_samplers)) {
        XR_LOG_ERR("Sampler state {} does not exist", sampler_name);
        return;
    }

    sampler_state_itr->ss_handle = sampler_state;
}

void
xray::rendering::shader_common_core::set_constant_buffers(
    ID3D11DeviceContext* device_context,
    void (ID3D11DeviceContext::*cbuff_set_fn)(UINT, UINT, ID3D11Buffer* const*))
{
    assert(_valid);
    assert(cbuff_set_fn != nullptr);

    if (_uniform_blocks.empty())
        return;

    stlsoft::auto_buffer<ID3D11Buffer*> bound_cbuffers{ _uniform_blocks.size() };
    auto bound_cbuff_itr = bound_cbuffers.begin();

    for (auto& block : _uniform_blocks) {

        //
        // Flush any data modifications to the GPU.
        if (block.sub_dirty) {
            const auto block_data = raw_ptr(_datastore) + block.sub_store_offset;
            device_context->UpdateSubresource(
                raw_ptr(block.sub_gpu_buffer), 0, nullptr, block_data, block.sub_bytes_size, block.sub_bytes_size);
            block.sub_dirty = false;
        }

        *bound_cbuff_itr++ = raw_ptr(block.sub_gpu_buffer);
    }

    (device_context->*cbuff_set_fn)(
        _st_slots.ss_cbuffer, static_cast<uint32_t>(bound_cbuffers.size()), bound_cbuffers.data());
}

void
xray::rendering::shader_common_core::set_resource_views(
    ID3D11DeviceContext* device_context,
    void (ID3D11DeviceContext::*rvs_set_fn)(UINT, UINT, ID3D11ShaderResourceView* const*))
{

    assert(device_context != nullptr);
    assert(rvs_set_fn != nullptr);
    assert(_valid);

    if (_resource_views.empty())
        return;

    stlsoft::auto_buffer<ID3D11ShaderResourceView*> bound_rvs{ _resource_views.size() };
    auto bound_rvs_itr = bound_rvs.begin();

    for (auto& srv : _resource_views) {
        *bound_rvs_itr++ = srv.srv_bound_view;
    }

    (device_context->*rvs_set_fn)(
        _st_slots.ss_resourceviews, static_cast<uint32_t>(bound_rvs.size()), bound_rvs.data());
}

void
xray::rendering::shader_common_core::set_samplers(
    ID3D11DeviceContext* device_context,
    void (ID3D11DeviceContext::*samplers_set_fn)(UINT, UINT, ID3D11SamplerState* const*))
{

    assert(_valid);
    assert(device_context != nullptr);
    assert(samplers_set_fn != nullptr);

    stlsoft::auto_buffer<ID3D11SamplerState*> bound_samplers{ _samplers.size() };
    auto bound_samplers_itr = bound_samplers.begin();

    for (auto& smp : _samplers)
        *bound_samplers_itr++ = smp.ss_handle;

    (device_context->*samplers_set_fn)(
        _st_slots.ss_samplers, static_cast<uint32_t>(bound_samplers.size()), bound_samplers.data());
}
