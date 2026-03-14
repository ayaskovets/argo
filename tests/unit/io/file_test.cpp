#include <wip/io/file.hpp>

#include <gtest/gtest.h>

namespace tests::io {

namespace io = wip::io;

TEST(io_file, size) {
  static_assert(sizeof(io::file) == 4);
  static_assert(alignof(io::file) == 4);
}

TEST(io_file, open) {
  const auto path = std::filesystem::current_path().append("io_file_open");

  std::filesystem::remove(path);
  EXPECT_ANY_THROW(io::file::open(path, io::file::access_mode::kRdonly));
  EXPECT_NO_THROW(io::file::create(path, io::file::access_mode::kRdonly));
  EXPECT_ANY_THROW(io::file::create(path, io::file::access_mode::kRdonly));
  EXPECT_NO_THROW(io::file::open(path, io::file::access_mode::kRdonly));
}

TEST(io_file, access_mode) {
  const auto path =
      std::filesystem::current_path().append("io_file_access_mode");
  const std::vector<std::byte> data{std::byte('a'), std::byte('b'),
                                    std::byte('c'), std::byte('d')};
  std::vector<std::byte> buffer(data.size());

  std::filesystem::remove(path);
  auto ro_file = io::file::create(path, io::file::access_mode::kRdonly);
  EXPECT_EQ(ro_file.get_access_mode(), io::file::access_mode::kRdonly);
  EXPECT_ANY_THROW(ro_file.write(data));
  EXPECT_EQ(ro_file.read(buffer), 0);

  std::filesystem::remove(path);
  auto wo_file = io::file::create(path, io::file::access_mode::kWronly);
  EXPECT_EQ(wo_file.get_access_mode(), io::file::access_mode::kWronly);
  EXPECT_EQ(wo_file.write(data), data.size());
  EXPECT_ANY_THROW(wo_file.read(buffer));

  std::filesystem::remove(path);
  auto rw_file = io::file::create(path, io::file::access_mode::kRdwr);
  EXPECT_EQ(rw_file.get_access_mode(), io::file::access_mode::kRdwr);
  EXPECT_EQ(rw_file.write(data), data.size());
  EXPECT_EQ(rw_file.read(buffer), 0);
}

TEST(io_file, offset) {
  const auto path = std::filesystem::current_path().append("io_file_offset");
  const std::vector<std::byte> data{std::byte('a'), std::byte('b'),
                                    std::byte('c'), std::byte('d')};
  std::vector<std::byte> buffer(data.size());

  std::filesystem::remove(path);
  auto file = io::file::create(path, io::file::access_mode::kRdwr);
  EXPECT_EQ(file.get_offset(), 0);

  EXPECT_EQ(file.write(data), data.size());
  EXPECT_EQ(file.get_offset(), data.size());

  file.set_offset(1, io::file::seek_whence::kBegin);
  EXPECT_EQ(file.get_offset(), 1);

  EXPECT_EQ(file.read(buffer), buffer.size() - 1);
  EXPECT_EQ(file.get_offset(), data.size());

  file.set_offset(1, io::file::seek_whence::kBegin);
  EXPECT_EQ(file.get_offset(), 1);

  file.set_offset(1, io::file::seek_whence::kCurrent);
  EXPECT_EQ(file.get_offset(), 2);

  file.set_offset(1, io::file::seek_whence::kEnd);
  EXPECT_EQ(file.get_offset(), data.size() + 1);
}

TEST(io_file, hole) {
  const auto path = std::filesystem::current_path().append("io_file_hole");
  const std::vector<std::byte> data{std::byte('a'), std::byte('b'),
                                    std::byte('c'), std::byte('d')};

  std::filesystem::remove(path);
  auto file = io::file::create(path, io::file::access_mode::kRdwr);

  EXPECT_EQ(file.write(data), data.size());

  file.set_offset(0, io::file::seek_whence::kBegin);
  EXPECT_EQ(file.get_offset(), 0);

  file.set_offset(0, io::file::seek_whence::kData);
  EXPECT_EQ(file.get_offset(), 0);

  file.set_offset(0, io::file::seek_whence::kHole);
  EXPECT_EQ(file.get_offset(), data.size());
}

TEST(io_file, truncate) {
  const auto path = std::filesystem::current_path().append("io_file_truncate");
  const std::vector<std::byte> data{std::byte('a'), std::byte('b'),
                                    std::byte('c'), std::byte('d')};

  std::filesystem::remove(path);
  auto file = io::file::create(path, io::file::access_mode::kRdwr, {},
                               io::file::flag::kSync);
  EXPECT_EQ(file.get_size_bytes(), 0);
  EXPECT_EQ(file.write(data), data.size());
  EXPECT_EQ(file.get_size_bytes(), data.size());
  EXPECT_EQ(file.write(data), data.size());
  EXPECT_EQ(file.get_size_bytes(), data.size() + data.size());

  file.truncate(2);
  EXPECT_EQ(file.get_size_bytes(), 2);
}

TEST(io_file, creation_flags) {
  const auto path =
      std::filesystem::current_path().append("io_file_creation_flags");

  for (const auto flag : std::initializer_list<io::file::open_flag>{
           io::file::open_flag::kCloexec, io::file::open_flag::kTrunc}) {
    std::filesystem::remove(path);
    EXPECT_ANY_THROW(
        io::file::open(path, io::file::access_mode::kRdonly, flag));
    EXPECT_NO_THROW(
        io::file::create(path, io::file::access_mode::kRdonly, flag));
    EXPECT_NO_THROW(io::file::open(path, io::file::access_mode::kRdonly, flag));
    std::filesystem::remove(path);
    EXPECT_ANY_THROW(
        io::file::open(path, io::file::access_mode::kRdonly, flag));
  }
}

TEST(io_file, flags) {
  const auto path = std::filesystem::current_path().append("io_file_flags");

  std::filesystem::remove(path);
  auto file = io::file::create(path, io::file::access_mode::kRdonly);

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
