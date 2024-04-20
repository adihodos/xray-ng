#include "misc/texture_array/texture_array_demo.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/array_dimension.hpp"
#include "xray/base/debug_output.hpp"
#include "xray/math/math_std.hpp"
#include "xray/math/scalar2.hpp"
#include "xray/rendering/colors/color_palettes.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/texture_loader.hpp"
#include "xray/ui/events.hpp"
#include <algorithm>
#include <cassert>
#include <imgui/imgui.h>
#include <opengl/opengl.hpp>
#include <string>

using namespace xray::base;
using namespace xray::rendering;
using namespace xray::math;
using namespace xray::ui;
using namespace std;

extern xray::base::app_config* xr_app_config;

struct imgui_scope {
  imgui_scope(const char* name = nullptr) { ImGui::Begin(name); }
  ~imgui_scope() { ImGui::End(); }
};

struct imgui_group_scope {
  imgui_group_scope() { ImGui::BeginGroup(); }
  ~imgui_group_scope() { ImGui::EndGroup(); }
};

struct vertex2pt {
  vec2f pos;
  vec2f texc;
};

static constexpr vertex2pt QUAD_VERTICES[] = {{{-1.0f, -1.0f}, {0.0f, 0.0f}},
                                              {{-1.0f, +1.0f}, {0.0f, 1.0f}},
                                              {{+1.0f, +1.0f}, {1.0f, 1.0f}},
                                              {{+1.0f, -1.0f}, {1.0f, 0.0f}}};

static constexpr uint8_t QUAD_INDICES[] = {0, 1, 2, 0, 2, 3};

static constexpr const char* TEXTURE_FILES[] = {"ash_uvgrid01.jpg",
                                                "ash_uvgrid02.jpg",
                                                "ash_uvgrid03.jpg",
                                                "ash_uvgrid04.jpg",
                                                "ash_uvgrid05.jpg",
                                                "ash_uvgrid06.jpg",
                                                "ash_uvgrid07.jpg",
                                                "ash_uvgrid08.jpg",
                                                "ash_uvgrid09.png",
                                                "ash_uvgrid10.jpg"};

app::texture_array_demo::texture_array_demo(const init_context_t& init_ctx)
  : app::demo_base{init_ctx} {
  _quad_vb = []() {
    GLuint buffer{};
    gl::CreateBuffers(1, &buffer);
    gl::NamedBufferStorage(
      buffer, (GLsizeiptr) sizeof(QUAD_VERTICES), QUAD_VERTICES, 0);
    return buffer;
  }();

  _quad_ib = []() {
    GLuint index_buff{};
    gl::CreateBuffers(1, &index_buff);
    gl::NamedBufferStorage(
      index_buff, (GLsizeiptr) sizeof(QUAD_INDICES), QUAD_INDICES, 0);
    return index_buff;
  }();

  _quad_layout = [ vb = raw_handle(_quad_vb), ib = raw_handle(_quad_ib) ]() {
    GLuint vao{};
    gl::CreateVertexArrays(1, &vao);
    gl::VertexArrayVertexBuffer(vao, 0, vb, 0, (GLsizei) sizeof(vertex2pt));
    gl::VertexArrayElementBuffer(vao, ib);

    gl::VertexArrayAttribFormat(
      vao, 0, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex2pt, pos));
    gl::VertexArrayAttribFormat(
      vao, 1, 2, gl::FLOAT, gl::FALSE_, XR_U32_OFFSETOF(vertex2pt, texc));

    gl::EnableVertexArrayAttrib(vao, 0);
    gl::EnableVertexArrayAttrib(vao, 1);

    gl::VertexArrayAttribBinding(vao, 0, 0);
    gl::VertexArrayAttribBinding(vao, 1, 0);

    return vao;
  }
  ();

  _vs = gpu_program_builder{}
          .add_file("shaders/misc/texture_array/shader.vert.glsl")
          .build<render_stage::e::vertex>();

  if (!_vs)
    return;

  _fs = gpu_program_builder{}
          .add_file("shaders/misc/texture_array/shader.frag.glsl")
          .build<render_stage::e::fragment>();

  if (!_fs)
    return;

  _pipeline = program_pipeline{[]() {
    GLuint ppl{};
    gl::CreateProgramPipelines(1, &ppl);
    return ppl;
  }()};

  _pipeline.use_vertex_program(_vs).use_fragment_program(_fs);

  _texture_array = []() {
    GLuint texture{};
    gl::CreateTextures(gl::TEXTURE_2D_ARRAY, 1, &texture);
    gl::TextureStorage3D(
      texture, 4, gl::RGBA8, 1024, 1024, XR_I32_COUNTOF(TEXTURE_FILES));

    int32_t z_offset{};
    for_each(
      begin(TEXTURE_FILES),
      end(TEXTURE_FILES),
      [texture, &z_offset](const char* texfile) {
        const auto full_path =
          xr_app_config->texture_path((string{"uv_grids/"} + texfile).c_str());

        texture_loader tex_ldr{full_path.c_str(), texture_load_options::flip_y};
        if (!tex_ldr) {
          XR_LOG_CRITICAL("Cannot open texture file {}", full_path.c_str());
          XR_NOT_REACHED();
        }

        gl::TextureSubImage3D(texture,
                              0,
                              0,
                              0,
                              z_offset++,
                              tex_ldr.width(),
                              tex_ldr.height(),
                              1,
                              tex_ldr.format(),
                              tex_ldr.pixel_size(),
                              tex_ldr.data());

        gl::GenerateTextureMipmap(texture);
      });

    return texture;
  }();

  gl::CreateSamplers(1, raw_handle_ptr(_sampler));
  gl::SamplerParameteri(
    raw_handle(_sampler), gl::TEXTURE_MIN_FILTER, _min_filter);
  gl::SamplerParameteri(
    raw_handle(_sampler), gl::TEXTURE_MAG_FILTER, _mag_filter);

  gl::SamplerParameteri(raw_handle(_sampler), gl::TEXTURE_WRAP_S, _wrap_mode);
  gl::SamplerParameteri(raw_handle(_sampler), gl::TEXTURE_WRAP_T, _wrap_mode);
  gl::SamplerParameterfv(
    raw_handle(_sampler), gl::TEXTURE_BORDER_COLOR, _border_color.components);

  _ui->set_global_font("Roboto-Regular");
  _valid = true;
}

app::texture_array_demo::~texture_array_demo() {}

void app::texture_array_demo::loop_event(
  const xray::ui::window_loop_event& wle) {
  _ui->tick(1.0f / 60.0f);
  draw(wle.wnd_width, wle.wnd_height);
  draw_ui(wle.wnd_width, wle.wnd_height);
}

void app::texture_array_demo::event_handler(const xray::ui::window_event& evt) {
  if (is_input_event(evt)) {
    _ui->input_event(evt);
    if (_ui->wants_input()) {
      return;
    }

    if (evt.type == event_type::key &&
        evt.event.key.keycode == key_sym::e::escape) {
      _quit_receiver();
      return;
    }
  }
}

void app::texture_array_demo::draw(const int32_t surface_width,
                                   const int32_t surface_height) {
  assert(valid());

  gl::ClearNamedFramebufferfv(
    0, gl::COLOR, 0, color_palette::web::black.components);
  gl::ViewportIndexedf(0,
                       0.0f,
                       0.0f,
                       static_cast<float>(surface_width),
                       static_cast<float>(surface_height));

  gl::BindTextureUnit(0, raw_handle(_texture_array));
  gl::BindSampler(0, raw_handle(_sampler));
  _vs.set_uniform("WRAPPING_FACTOR", _wrap_factor);
  _fs.set_uniform("TEX_IMAGE_IDX", _image_index);

  gl::BindVertexArray(raw_handle(_quad_layout));
  _pipeline.use();
  gl::DrawElements(gl::TRIANGLES, 6, gl::UNSIGNED_BYTE, nullptr);
}

struct texture_param_info {
  GLenum      mode;
  const char* desc;
};

#define TEX_PARAM_INFO_ENTRY(type)                                             \
  { type, #type }

static constexpr texture_param_info TEXTURE_WRAP_MODES[] = {
  TEX_PARAM_INFO_ENTRY(gl::REPEAT),
  TEX_PARAM_INFO_ENTRY(gl::CLAMP_TO_EDGE),
  TEX_PARAM_INFO_ENTRY(gl::CLAMP_TO_BORDER),
  TEX_PARAM_INFO_ENTRY(gl::MIRRORED_REPEAT),
  TEX_PARAM_INFO_ENTRY(gl::MIRROR_CLAMP_TO_EDGE)};

static constexpr texture_param_info TEXTURE_SAMPLING_MODES[] = {
  TEX_PARAM_INFO_ENTRY(gl::NEAREST),
  TEX_PARAM_INFO_ENTRY(gl::LINEAR),
  TEX_PARAM_INFO_ENTRY(gl::NEAREST_MIPMAP_NEAREST),
  TEX_PARAM_INFO_ENTRY(gl::NEAREST_MIPMAP_LINEAR),
  TEX_PARAM_INFO_ENTRY(gl::LINEAR_MIPMAP_NEAREST),
  TEX_PARAM_INFO_ENTRY(gl::LINEAR_MIPMAP_LINEAR)};

void app::texture_array_demo::draw_ui(const int32_t surface_width,
                                      const int32_t surface_height) {
  _ui->new_frame(surface_width, surface_height);

  ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Appearing);
  if (ImGui::Begin("Options",
                   nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_ShowBorders)) {

    if (ImGui::Button("<< Previous image")) {
      XR_LOG_INFO("Pushed button!");
	  using xray::math::clamp;
      _image_index =
        clamp(_image_index - 1, 0, XR_I32_COUNTOF(TEXTURE_FILES) - 1);
    }

    if (ImGui::Button("Next image >>")) {
      XR_LOG_INFO("Pushed button!");
	  using xray::math::clamp;
      _image_index =
        clamp(_image_index + 1, 0, XR_I32_COUNTOF(TEXTURE_FILES) - 1);
    }

    ImGui::Separator();

    {
      imgui_group_scope grp{};
      ImGui::Text("Texture wrapping");
      for_each(begin(TEXTURE_WRAP_MODES),
               end(TEXTURE_WRAP_MODES),
               [this](const auto& twm) {
                 const auto selected =
                   ImGui::RadioButton(twm.desc, _wrap_mode == twm.mode);

                 if (!selected)
                   return;

                 if (_wrap_mode == twm.mode)
                   return;

                 _wrap_mode = twm.mode;
                 gl::SamplerParameteri(
                   raw_handle(_sampler), gl::TEXTURE_WRAP_S, _wrap_mode);
                 gl::SamplerParameteri(
                   raw_handle(_sampler), gl::TEXTURE_WRAP_T, _wrap_mode);
               });

      if (_wrap_mode == gl::CLAMP_TO_BORDER) {
        const bool color_changed =
          ImGui::ColorEdit4("Border color", _border_color.components, false);
        if (color_changed) {
          gl::SamplerParameterfv(raw_handle(_sampler),
                                 gl::TEXTURE_BORDER_COLOR,
                                 _border_color.components);
        }
      }

      ImGui::SliderInt("Texture wrapping factor", &_wrap_factor, 1, 10, "%.0f");
    }

    ImGui::Separator();
    {
      imgui_group_scope grp{};
      ImGui::Text("Minification filter");

      for_each(begin(TEXTURE_SAMPLING_MODES),
               end(TEXTURE_SAMPLING_MODES),
               [this](const auto& samp_mode) {
                 const auto sel_changed = ImGui::RadioButton(
                   samp_mode.desc, samp_mode.mode == _min_filter);
                 if (!sel_changed || (samp_mode.mode == _min_filter))
                   return;

                 _min_filter = samp_mode.mode;
                 gl::SamplerParameteri(
                   raw_handle(_sampler), gl::TEXTURE_MIN_FILTER, _min_filter);
               });
    }

    ImGui::Separator();
    {
      imgui_group_scope grp{};
      ImGui::Text("Magnification filter");

      int32_t id{};
      for_each(begin(TEXTURE_SAMPLING_MODES),
               begin(TEXTURE_SAMPLING_MODES) + 2,
               [this, &id](const auto& samp_mode) {
                 ImGui::PushID(id++);

                 const auto sel_changed = ImGui::RadioButton(
                   samp_mode.desc, samp_mode.mode == _mag_filter);

                 ImGui::PopID();
                 if (!sel_changed || (samp_mode.mode == _mag_filter))
                   return;

                 _mag_filter = samp_mode.mode;
                 gl::SamplerParameteri(
                   raw_handle(_sampler), gl::TEXTURE_MAG_FILTER, _mag_filter);
               });
    }
  }

  ImGui::End();
  _ui->draw();
}
