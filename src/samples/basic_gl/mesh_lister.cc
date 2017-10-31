//
// Copyright (c) 2011-2017 Adrian Hodos
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

#include "mesh_lister.hpp"
#include <algorithm>
#include <cassert>
#include <platformstl/filesystem/filesystem_traits.hpp>
#include <platformstl/filesystem/path.hpp>
#include <platformstl/filesystem/readdir_sequence.hpp>

using namespace std;

void app::mesh_lister::build_list(const char* root_path) {
  assert(root_path != nullptr);
  build_list_impl(root_path);
  sort(begin(_mesh_list),
       end(_mesh_list),
       [](const model_load_info& m0, const model_load_info& m1) {
         return m0.name < m1.name;
       });
}

void app::mesh_lister::build_list_impl(const char* root_path) {
  platformstl::readdir_sequence rddir{
    root_path,
    platformstl::readdir_sequence::files |
      platformstl::readdir_sequence::directories |
      platformstl::readdir_sequence::fullPath};

  for_each(begin(rddir), end(rddir), [this](const char* dir_entry) {
    if (platformstl::filesystem_traits<char>::is_directory(dir_entry)) {
      build_list(dir_entry);
      return;
    }

    platformstl::path_a p{dir_entry};
    if (strcmp(p.get_ext(), "bin") == 0) {
      _mesh_list.push_back({p.pop_ext().get_file(), dir_entry});
    }
  });
}
