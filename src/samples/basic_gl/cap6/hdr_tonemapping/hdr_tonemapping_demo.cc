#include "cap6/hdr_tonemapping/hdr_tonemapping_demo.hpp"
#include "helpers.hpp"
#include "init_context.hpp"
#include "resize_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/config_settings.hpp"
#include "xray/base/dbg/debug_ext.hpp"
#include "xray/base/logger.hpp"
#include "xray/base/shims/attribute/basic_path.hpp"
#include "xray/math/scalar4x4_math.hpp"
#include "xray/math/transforms_r3.hpp"
#include "xray/rendering/draw_context.hpp"
#include "xray/rendering/texture_loader.hpp"
#include <algorithm>
#include <imgui/imgui.h>
#include <platformstl/filesystem/memory_mapped_file.hpp>
#include <platformstl/filesystem/path.hpp>
#include <platformstl/filesystem/path_functions.hpp>

extern xray::base::app_config* xr_app_config;

using namespace xray::base;
using namespace xray::math;
using namespace xray::rendering;
using namespace xray::scene;
using namespace std;
using namespace app;

app::hdr_tonemap::hdr_tonemap(const init_context_t& init_ctx) {
  init(init_ctx);
}

app::hdr_tonemap::~hdr_tonemap() {}

void app::hdr_tonemap::compose_ui() {
  if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize |
                                            ImGuiWindowFlags_NoMove)) {
  }

  ImGui::End();
}

void app::hdr_tonemap::draw(const xray::rendering::draw_context_t& dc) {
  assert(valid());
}

void app::hdr_tonemap::update(const float /*delta_ms*/) {}

void app::hdr_tonemap::key_event(const int32_t /*key_code*/,
                                 const int32_t /*action*/,
                                 const int32_t /*mods*/) {}

void app::hdr_tonemap::resize_event(const resize_context_t& resize_ctx) {
  assert(valid());

  const auto r_width  = static_cast<GLsizei>(resize_ctx.surface_width);
  const auto r_height = static_cast<GLsizei>(resize_ctx.surface_height);

  //
  // resize the framebuffer
  create_framebuffer(r_width, r_height);
}

void app::hdr_tonemap::create_framebuffer(const GLsizei r_width,
                                          const GLsizei r_height) {
  struct fbo_data {
    xray::rendering::scoped_framebuffer  fbo_object;
    xray::rendering::scoped_renderbuffer fbo_depth;
    xray::rendering::scoped_texture      fbo_texture;
    xray::rendering::scoped_sampler      fbo_sampler;
  } _fbo;

  if (!_fbo.fbo_sampler) {
    gl::CreateSamplers(1, &raw_handle_ptr(_fbo.fbo_sampler));
    gl::SamplerParameteri(raw_handle(_fbo.fbo_sampler), gl::TEXTURE_MIN_FILTER,
                          gl::LINEAR);
    gl::SamplerParameteri(raw_handle(_fbo.fbo_sampler), gl::TEXTURE_MAG_FILTER,
                          gl::LINEAR);
  }

  _fbo.fbo_object  = scoped_framebuffer{};
  _fbo.fbo_depth   = scoped_renderbuffer{};
  _fbo.fbo_texture = scoped_texture{};

  {
    gl::CreateTextures(gl::TEXTURE_2D, 1, raw_handle_ptr(_fbo.fbo_texture));
    gl::TextureStorage2D(raw_handle(_fbo.fbo_texture), 1, gl::RGBA32F, r_width,
                         r_height);
  }

  {
    gl::CreateRenderbuffers(1, raw_handle_ptr(_fbo.fbo_depth));
    gl::NamedRenderbufferStorage(raw_handle(_fbo.fbo_depth),
                                 gl::DEPTH_COMPONENT32F, r_width, r_height);
  }

  {
    gl::CreateFramebuffers(1, raw_handle_ptr(_fbo.fbo_object));
    gl::NamedFramebufferTexture(raw_handle(_fbo.fbo_object),
                                gl::COLOR_ATTACHMENT0,
                                raw_handle(_fbo.fbo_texture), 0);
    gl::NamedFramebufferRenderbuffer(raw_handle(_fbo.fbo_object),
                                     gl::DEPTH_ATTACHMENT, gl::RENDERBUFFER,
                                     raw_handle(_fbo.fbo_depth));
    gl::NamedFramebufferDrawBuffer(raw_handle(_fbo.fbo_object),
                                   gl::COLOR_ATTACHMENT0);
  }

  const auto fbo_status = gl::CheckNamedFramebufferStatus(
      raw_handle(_fbo.fbo_object), gl::FRAMEBUFFER);

  if (fbo_status != gl::FRAMEBUFFER_COMPLETE) {
    XR_LOG_CRITICAL("Framebuffer is not complete!");
    XR_NOT_REACHED();
  }
}

void app::hdr_tonemap::init(const init_context_t& ini_ctx) {

  const auto render_wnd_width  = static_cast<GLsizei>(ini_ctx.surface_width);
  const auto render_wnd_height = static_cast<GLsizei>(ini_ctx.surface_height);

  create_framebuffer(render_wnd_width, render_wnd_height);

  _valid = true;
}
