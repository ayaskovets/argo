#include <sandbox/io/file.hpp>

#include <gtest/gtest.h>

namespace tests::io {

namespace io = sandbox::io;

// TODO: test read & write & seek & tell & truncate & size

TEST(io_file, creation_flags) {
  const auto path =
      std::filesystem::current_path().append("io_file_creation_flags");

  for (const auto flag : std::initializer_list<io::file::open_flag>{
           io::file::open_flag::kCloexec, io::file::open_flag::kTrunc}) {
    std::filesystem::remove(path);
    EXPECT_ANY_THROW(
        io::file::open(path, flag, {}, io::file::access_mode::kRdonly));
    EXPECT_NO_THROW(io::file::create(path, flag, {},
                                     io::file::access_mode::kRdonly,
                                     std::filesystem::perms::owner_all));
    EXPECT_NO_THROW(
        io::file::open(path, flag, {}, io::file::access_mode::kRdonly));
    std::filesystem::remove(path);
    EXPECT_ANY_THROW(
        io::file::open(path, flag, {}, io::file::access_mode::kRdonly));
  }
}

TEST(io_file, flags) {
  const auto path = std::filesystem::current_path().append("io_file_flags");

  std::filesystem::remove(path);
  auto file = io::file::create(path, {}, {}, io::file::access_mode::kRdonly,
                               std::filesystem::perms::owner_all);

  std::common_type_t<io::fd::flags_t, io::file::flags_t> flags{};
  EXPECT_EQ(file.get_flags(), flags);
  for (const auto flag : std::initializer_list<
           std::common_type_t<io::fd::flags_t, io::file::flags_t>>{
           io::fd::flag::kNonblock, io::file::flag::kAppend,
           io::file::flag::kSync, io::file::flag::kDsync}) {
    flags |= flag;
    file.set_flags(flags);
    EXPECT_EQ(file.get_flags(), flags);
  }
}

}  // namespace tests::io
