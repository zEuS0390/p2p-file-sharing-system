#ifndef CORE_TYPES_FILE_INFORMATION_HPP
#define CORE_TYPES_FILE_INFORMATION_HPP

#include <cstdint>
#include <string>

struct FileInformation
{
  std::string file_name;
  uint64_t file_size;
};

#endif
