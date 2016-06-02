#pragma once

#include "xray/xray.hpp"
#include "xray/rendering/opengl/gl_handles.hpp"
#include "xray/rendering/opengl/gpu_program.hpp"
#include "xray/rendering/rendering_fwd.hpp"
#include <cstdint>

namespace app {

class frag_discard_demo {
public :
    frag_discard_demo();

    ~frag_discard_demo();

    void draw(const xray::rendering::draw_context_t& dc) noexcept;
    void update(const float delta_ms) noexcept;
    void key_event(const int32_t key_code, const int32_t action,
                   const int32_t mods) noexcept;


    bool valid() const noexcept {
        return valid_;
    }

    explicit operator bool() const noexcept {
        return valid();
    }

private :
    void init();

private :
    xray::rendering::scoped_buffer vertex_buff_;
    xray::rendering::scoped_buffer index_buff_;
    xray::rendering::scoped_vertex_array vertex_layout_;
    xray::rendering::gpu_program draw_prog_;
    uint32_t mesh_indices_{0};
    bool valid_{false};

private :
    XRAY_NO_COPY(frag_discard_demo);
};

}
