#ifndef ZOS_FILESYSTEM_H
#define ZOS_FILESYSTEM_H

#include <string.h>
#include <cstdint>

std::uintmax_t portable_remove_all(const std::string& path);

#endif
