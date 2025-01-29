//
// Copyright (c) Adrian Hodos
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

#pragma once

#include "xray/rendering/vulkan.renderer/vulkan.unique.resource.hpp"

namespace xray::rendering {

class VulkanRenderer;

struct QueueSubmitWaitToken
{
  public:
    QueueSubmitWaitToken(VkCommandBuffer cmd_buf, xrUniqueVkFence&& fence, VulkanRenderer* renderer) noexcept
        : _cmdbuf{ cmd_buf }
        , _fence{ xray::base::unique_pointer_release(fence) }
        , _renderer{ renderer }
    {
    }

    QueueSubmitWaitToken(QueueSubmitWaitToken&& rhs) noexcept
        : _cmdbuf{ std::exchange(rhs._cmdbuf, VK_NULL_HANDLE) }
        , _fence{ std::exchange(rhs._fence, VK_NULL_HANDLE) }
        , _renderer{ rhs._renderer }
    {
    }

    QueueSubmitWaitToken(const QueueSubmitWaitToken&) = delete;
    QueueSubmitWaitToken& operator=(const QueueSubmitWaitToken&&) = delete;

    QueueSubmitWaitToken& operator=(QueueSubmitWaitToken&& rhs) noexcept
    {
        std::exchange(_cmdbuf, rhs._cmdbuf);
        std::exchange(_fence, rhs._fence);
        std::exchange(_renderer, rhs._renderer);
        return *this;
    }

    ~QueueSubmitWaitToken() { dispose(); }

    void dispose() noexcept;
    void wait() noexcept;
    VkCommandBuffer command_buffer() const noexcept { return _cmdbuf; }
    VkFence fence() const noexcept { return _fence; }

  private:
    VkCommandBuffer _cmdbuf;
    VkFence _fence;
    VulkanRenderer* _renderer;
};

}
