//
// Copyright (c) 2011, 2012, 2013 Adrian Hodos
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

#include "demo_base.hpp"
#include "init_context.hpp"
#include "xray/base/app_config.hpp"
#include "xray/base/debug_output.hpp"
#include <algorithm>
#include <platformstl/filesystem/filesystem_traits.hpp>
#include <platformstl/filesystem/readdir_sequence.hpp>
#include <vector>

using namespace std;

void build_fonts_list(const char*                       root_path,
                      std::vector<xray::ui::font_info>* fli) {
  platformstl::readdir_sequence rddir{
    root_path,
    platformstl::readdir_sequence::fullPath |
      platformstl::readdir_sequence::directories |
      platformstl::readdir_sequence::files};

  for_each(begin(rddir), end(rddir), [fli](const char* dir_entry) {
    if (platformstl::filesystem_traits<char>::is_directory(dir_entry)) {
      build_fonts_list(dir_entry, fli);
      return;
    }

    fli->push_back({dir_entry, 18.0f});
  });
}

app::demo_base::demo_base(const init_context_t& init_ctx)
  : _quit_receiver{init_ctx.quit_receiver} {
  //
  // load default fonts
  vector<xray::ui::font_info> fli;
  build_fonts_list(init_ctx.cfg->font_root().c_str(), &fli);
  _ui =
    xray::base::make_unique<xray::ui::user_interface>(fli.data(), fli.size());
}

app::demo_base::~demo_base() {}

void app::demo_base::poll_start(const xray::ui::poll_start_event&) {}

void app::demo_base::poll_end(const xray::ui::poll_end_event&) {}
