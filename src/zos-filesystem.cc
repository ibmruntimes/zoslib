#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <cerrno>
#include <stdexcept>
#include <zos-filesystem.h>

std::uintmax_t portable_remove_all(const std::string& path) {
  std::uintmax_t count = 0;
  struct stat path_stat{};

  if (lstat(path.c_str(), &path_stat) != 0) {
    if (errno == ENOENT) return 0;
    throw std::runtime_error("lstat failed: " + path + ": " + std::strerror(errno));
  }

  if (S_ISDIR(path_stat.st_mode)) {
    DIR* dir = opendir(path.c_str());
    if (!dir) throw std::runtime_error("opendir failed: " + path + ": " + std::strerror(errno));

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string name = entry->d_name;
      if (name == "." || name == "..") continue;
      count += portable_remove_all(path + "/" + name);
    }
    closedir(dir);
    if (rmdir(path.c_str()) != 0) {
      throw std::runtime_error("rmdir failed: " + path + ": " + std::strerror(errno));
    }
    count++;
  } 
  else {
    if (unlink(path.c_str()) != 0) {
      throw std::runtime_error("unlink failed: " + path + ": " + std::strerror(errno));
    }
    count++;
  }

  return count;
}
