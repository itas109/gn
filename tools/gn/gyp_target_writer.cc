// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/gyp_target_writer.h"

#include <iostream>

#include "base/file_util.h"
#include "base/files/file_path.h"
#include "tools/gn/build_settings.h"
#include "tools/gn/builder_record.h"
#include "tools/gn/filesystem_utils.h"
#include "tools/gn/gyp_binary_target_writer.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/settings.h"
#include "tools/gn/target.h"

GypTargetWriter::GypTargetWriter(const Target* target, std::ostream& out)
    : settings_(target->settings()),
      target_(target),
      out_(out) {
}

GypTargetWriter::~GypTargetWriter() {
}

// static
void GypTargetWriter::WriteFile(const SourceFile& gyp_file,
                                const std::vector<TargetGroup>& targets,
                                Err* err) {
  if (targets.empty())
    return;
  const Settings* debug_settings =
      targets[0].debug->item()->AsTarget()->settings();
  const BuildSettings* debug_build_settings = debug_settings->build_settings();

  std::stringstream file;
  file << "# Generated by GN. Do not edit.\n\n";
  file << "{\n";
  file << "  'skip_includes': 1,\n";

  if (debug_settings->IsMac()) {
    // Global settings for make/ninja. This must match common.gypi :(
    file << "  'make_global_settings': [\n";
    file << "    ['CC', 'third_party/llvm-build/Release+Asserts/bin/clang'],\n";
    file <<
        "    ['CXX', 'third_party/llvm-build/Release+Asserts/bin/clang++'],\n";
    file << "    ['CC.host', '$(CC)'],\n";
    file << "    ['CXX.host', '$(CXX)'],\n";
    file << "  ],\n";
  }
  // TODO(brettw) android.

  file << "  'targets': [\n";

  for (size_t i = 0; i < targets.size(); i++) {
    const Target* cur = targets[i].debug->item()->AsTarget();
    switch (cur->output_type()) {
      case Target::COPY_FILES:
      case Target::CUSTOM:
      case Target::GROUP:
        break;  // TODO(brettw)
      case Target::EXECUTABLE:
      case Target::STATIC_LIBRARY:
      case Target::SHARED_LIBRARY:
      case Target::SOURCE_SET: {
        GypBinaryTargetWriter writer(targets[i], file);
        writer.Run();
        break;
      }
      default:
        CHECK(0);
    }
  }

  file << "  ],\n}\n";

  base::FilePath gyp_file_path = debug_build_settings->GetFullPath(gyp_file);
  std::string contents = file.str();
  file_util::WriteFile(gyp_file_path, contents.c_str(),
                       static_cast<int>(contents.size()));
}

