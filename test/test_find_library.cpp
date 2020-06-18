// Copyright 2019 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdlib.h>

#include <string>

#include "gtest/gtest.h"

#include "rcutils/get_env.h"
#include "rcutils/filesystem.h"
#include "rcpputils/find_library.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace rcpputils
{
namespace
{

TEST(test_find_library, find_library)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  // Get ground-truth values from CTest properties.
  std::string expected_library_path;
  {
    const char * _expected_library_path{};
    EXPECT_EQ(rcutils_get_env("_TEST_LIBRARY", &_expected_library_path), nullptr);
    EXPECT_NE(_expected_library_path, nullptr);
    expected_library_path = _expected_library_path;
  }

  const char * test_lib_dir{};
  EXPECT_EQ(rcutils_get_env("_TEST_LIBRARY_DIR", &test_lib_dir), nullptr);
  EXPECT_NE(test_lib_dir, nullptr);

  // Set our relevant path variable.
  const char * env_var{};
#ifdef _WIN32
  env_var = "PATH";
#elif __APPLE__
  env_var = "DYLD_LIBRARY_PATH";
#else
  env_var = "LD_LIBRARY_PATH";
#endif

#ifdef _WIN32
  EXPECT_EQ(_putenv_s(env_var, test_lib_dir), 0);
#else
  const int override = 1;
  EXPECT_EQ(setenv(env_var, test_lib_dir, override), 0);
#endif

  // Positive test.
  const std::string test_lib_actual = find_library_path("test_library");
  EXPECT_EQ(test_lib_actual, expected_library_path);

  // (Hopefully) Negative test.
  const std::string bad_path = find_library_path(
    "this_is_a_junk_libray_name_please_dont_define_this_if_you_do_then_"
    "you_are_really_naughty");
  EXPECT_EQ(bad_path, "");

#ifdef _WIN32
  // On Windows, test find library in the current directory, to support containerized deployments

  const char* currentDir = rcutils_join_path(test_lib_dir, "test_current_path", allocator);
  EXPECT_NE(currentDir, nullptr);

  const char* testFile = rcutils_join_path(currentDir, "test_win_current_dir.dll", allocator);
  EXPECT_NE(testFile, nullptr);

  // move a file into the non-current path
  EXPECT_NE(rcutils_mkdir(currentDir), false);
  EXPECT_NE(CopyFile(test_lib_actual.c_str(), currentDir, FALSE), FALSE);

  const std::string test_current_dir_fail = find_library_path("test_win_current_dir");
  EXPECT_EQ(test_current_dir_fail.empty(), true);  // shouldn't be found until we set the current directory.

  DWORD len = GetCurrentDirectoryA(0, NULL) + 1;  // How much space is needed for current path?
  char* originalCurrentDir = new char[len];
  GetCurrentDirectoryA(len, originalCurrentDir);

  SetCurrentDirectory(currentDir);

  const std::string test_current_dir_succeed = find_library_path("test_win_current_dir");
  EXPECT_EQ(test_current_dir_succeed.empty(), false);

  SetCurrentDirectory(currentDir);

  delete [] originalCurrentDir;
#endif
}

}  // namespace
}  // namespace rcpputils
