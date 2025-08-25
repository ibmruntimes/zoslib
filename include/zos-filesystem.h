#ifndef ZOS_FILESYSTEM_H
#define ZOS_FILESYSTEM_H

#ifdef __cplusplus
#include <string.h>
#include <cstdint>

std::uintmax_t portable_remove_all(const std::string& path);
#endif
#endif
