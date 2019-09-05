/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "path.h"
#include "utilities.h"
#include "types.h"
#include "filesystem.h"
#include "jsexport.h"

#include "platform.h"

using namespace mga;

// https://nodejs.org/api/fs.html#fs_class_fs_stats
class Stats : public JSExport {
public:
#ifdef _MSC_VER
  enum Flags {
    IFMT = _S_IFMT,
    IFIFO = _S_IFIFO,
    IFCHR = _S_IFCHR,
    IFDIR = _S_IFDIR,
    IFBLK= 0,
    IFREG = _S_IFREG,
    IFLNK = 0,
    IFSOCK = 0,
  };
#else
  enum Flags {
    IFMT = S_IFMT,
    IFIFO = S_IFIFO,
    IFCHR = S_IFCHR,
    IFDIR = S_IFDIR,
    IFBLK= S_IFBLK,
    IFREG = S_IFREG,
    IFLNK = S_IFLNK,
    IFSOCK = S_IFSOCK,
  };
#endif

  //--------------------------------------------------------------------------------------------------------------------

  Stats(std::string path, bool followLinks) {
    _stat = Stat::get(path, followLinks);
  };

  //--------------------------------------------------------------------------------------------------------------------

  static void registerInContext(ScriptingContext &context, JSObject &exports) {
    std::ignore = context;
    std::ignore = context;
    exports.defineClass("Stats", "", 2, [](JSObject *instance, JSValues &args) {
      // parameters: path, flag
      std::string path = args.as(ValueType::String, 0);
      bool followLinks = args.get(1);
      try {
        instance->setBacking(new Stats(path, followLinks));
      } catch (std::runtime_error &e) {
        std::string message = "'" + path + "': " + e.what();
        args.context()->throwScriptingError(ScriptingError::Error, message);
      }
    }, [](JSObject &prototype) {
      prototype.defineFunction({ "isFile" }, 0, [](JSExport *instance, JSValues &args) {
        args.pushResult(dynamic_cast<Stats *>(instance)->modeIs(Flags::IFREG));
      });
      prototype.defineFunction({ "isDirectory" }, 0, [](JSExport *instance, JSValues &args) {
        args.pushResult(dynamic_cast<Stats *>(instance)->modeIs(Flags::IFDIR));
      });
      prototype.defineFunction({ "isBlockDevice" }, 0, [](JSExport *instance, JSValues &args) {
        args.pushResult(dynamic_cast<Stats *>(instance)->modeIs(Flags::IFBLK));
      });
      prototype.defineFunction({ "isCharacterDevice" }, 0, [](JSExport *instance, JSValues &args) {
        args.pushResult(dynamic_cast<Stats *>(instance)->modeIs(Flags::IFCHR));
      });
      prototype.defineFunction({ "isSymbolicLink" }, 0, [](JSExport *instance, JSValues &args) {
        args.pushResult(dynamic_cast<Stats *>(instance)->modeIs(Flags::IFLNK));
      });
      prototype.defineFunction({ "isFIFO" }, 0, [](JSExport *instance, JSValues &args) {
        args.pushResult(dynamic_cast<Stats *>(instance)->modeIs(Flags::IFIFO));
      });
      prototype.defineFunction({ "isSocket" }, 0, [](JSExport *instance, JSValues &args) {
        args.pushResult(dynamic_cast<Stats *>(instance)->modeIs(Flags::IFSOCK));
      });

      prototype.defineVirtualProperty("dev", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getDev();
      }, nullptr);

      prototype.defineVirtualProperty("ino", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getIno();
      }, nullptr);

      prototype.defineVirtualProperty("mode", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getMode();
      }, nullptr);

      prototype.defineVirtualProperty("nlink", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getNLink();
      }, nullptr);

      prototype.defineVirtualProperty("uid", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getUid();
      }, nullptr);

      prototype.defineVirtualProperty("gid", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getGid();
      }, nullptr);

      prototype.defineVirtualProperty("rdev", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getRdev();
      }, nullptr);

      prototype.defineVirtualProperty("size", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getSize();
      }, nullptr);

      prototype.defineVirtualProperty("blksize", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getBlockkSize();
      }, nullptr);

      prototype.defineVirtualProperty("blocks", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getBlocks();
      }, nullptr);

      prototype.defineVirtualProperty("atimeMs", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getTimeMS(Stat::TimeSpecs::atime);
      }, nullptr);

      prototype.defineVirtualProperty("mtimeMs", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getTimeMS(Stat::TimeSpecs::mtime);
      }, nullptr);

      prototype.defineVirtualProperty("ctimeMs", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getTimeMS(Stat::TimeSpecs::ctime);
      }, nullptr);

      prototype.defineVirtualProperty("birthtimeMs", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getTimeMS(Stat::TimeSpecs::birthdate);
      }, nullptr);

      prototype.defineVirtualProperty("atime", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getTimeString(Stat::TimeSpecs::atime);
      }, nullptr);

      prototype.defineVirtualProperty("mtime", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getTimeString(Stat::TimeSpecs::mtime);
      }, nullptr);

      prototype.defineVirtualProperty("ctime", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getTimeString(Stat::TimeSpecs::ctime);
      }, nullptr);

      prototype.defineVirtualProperty("birthtime", [](ScriptingContext *, JSExport * instance, std::string const&) {
        return dynamic_cast<Stats *>(instance)->_stat->getTimeString(Stat::TimeSpecs::birthdate);
      }, nullptr);
    });
  }

  //--------------------------------------------------------------------------------------------------------------------

private:
  std::unique_ptr<Stat> _stat;
  bool modeIs(Flags flag) {
    return (_stat->getMode() & S_IFMT) == flag;
  }
};

//----------------- FS -------------------------------------------------------------------------------------------------

/**
 * Loads the (text) content from the given file. Throws a runtime exception if that wasn't possible.
 */
std::string FS::contentFromFile(std::string const& fileName) {
#ifdef _MSC_VER
  std::ifstream stream(Utilities::s2ws(fileName), std::ios::binary);
#else
  std::ifstream stream(fileName, std::ios::binary);
#endif
  if (!stream.good()) {
    throw std::runtime_error(Utilities::getLastError() + " (" + fileName + ")");
  }

  std::string text(std::istreambuf_iterator<char>{stream}, {});

  return text;
}

//----------------------------------------------------------------------------------------------------------------------

static void copyFile(std::string const& source, std::string const& target, CopyOptions options) {
  if (!options.filter(source))
    return;

#ifdef _MSC_VER
  auto sourceFilename = Utilities::s2ws(source);
  std::ifstream input(sourceFilename, std::ios::binary);
#else
  std::ifstream input(source, std::ios::binary);
#endif
  if (input.fail())
    throw std::runtime_error("Error while opening source file: '" + source + "'");

#ifdef _MSC_VER
  auto targetFilename = Utilities::s2ws(target);
  std::ofstream output(targetFilename, std::ios::binary);
#else
  std::ofstream output(target, std::ios::binary);
#endif
  if (output.fail())
    throw std::runtime_error("Error while opening target file: '" + target + "'");

  output << input.rdbuf();

  output.close();
  if (output.bad())
    throw std::runtime_error("Error while writing target file: '" + target + "'");

  if (options.preserveTimestamps) {
#ifdef _MSC_VER
    struct _stat statBuffer;
    if (_wstat(sourceFilename.c_str(), &statBuffer) == 0) {
      _utimbuf buffer;
      buffer.actime = statBuffer.st_atime;
      buffer.modtime = statBuffer.st_mtime;
      if (_wutime(targetFilename.c_str(), &buffer) == -1)
        throw std::runtime_error("Error while setting times to target file('" + target + "'): " + Utilities::getLastError());
    }
#elif defined(__APPLE__)
    struct stat statBuffer;
    if (stat(source.c_str(), &statBuffer) == 0) {
      utimbuf buffer;
      buffer.actime = statBuffer.st_atimespec.tv_sec;
      buffer.modtime = statBuffer.st_mtimespec.tv_sec;
      if (utime(target.c_str(), &buffer) == -1)
        throw std::runtime_error("Error while setting times to target file('" + target + "'): " + Utilities::getLastError());
    }
#else
    struct stat statBuffer;
    if (stat(source.c_str(), &statBuffer) == 0) {
      struct timespec times[2];
      times[0].tv_sec = statBuffer.st_atim.tv_sec;
      times[0].tv_nsec = statBuffer.st_atim.tv_nsec;
      times[1].tv_sec = statBuffer.st_mtim.tv_sec;
      times[1].tv_nsec = statBuffer.st_mtim.tv_nsec;
      int fd = open(target.c_str(), O_WRONLY);
      int result = futimens(fd, times);
      close(fd);
      if (result == -1)
        throw std::runtime_error("Error while setting times to target file('" + target + "'): " + Utilities::getLastError());
    }
#endif
  }
};

//----------------------------------------------------------------------------------------------------------------------

static void copyDir(std::string const& sourceDir, std::string const& targetDir, CopyOptions options) {
  FS::ensureDir(targetDir);

  for(auto &entry : FS::readDir(sourceDir)) {
    std::string source = Path::join({ sourceDir, entry });
    std::string target = Path::join({ targetDir, entry });

    if (FS::isDir(source))
      copyDir(source, target, options);
    else {
      if (FS::pathExists(target)) {
        if (!options.overwrite) {
          if (options.errorOnExist)
            throw std::runtime_error("Target file already exists (" + target +")");

          continue;
        }
      }
      
      copyFile(source, target, options);
    }
  }
};

//----------------------------------------------------------------------------------------------------------------------

void FS::copy(std::string const& source, std::string const& target, CopyOptions options) {
  if (!pathExists(source))
    return;

  // TODO: symlink handling
  if (isDir(source)) {
    copyDir(source, target, options);
    return;
  }

  if (pathExists(target)) {
    if (!options.overwrite) {
      if (options.errorOnExist)
        throw std::runtime_error("Target file already exists (" + target +")");

      return;
    }
  }

  copyFile(source, target, options);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Removes all content from the given directory. The directy itself stays and is created if it doesn't exist yet.
 */
void FS::emptyDir(std::string const& directory) {
  std::string path = Path::normalize(directory);
  if (!isDir(path)) {
    ensureDir(path);
    return;
  }

  for (auto &file : readDir(path))
    remove(Path::join({ path, file }));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Ensures that all folders in the given path exist.
 */
void FS::ensureDir(std::string const& directory) {
  if (isDir(directory))
    return;

  std::string currentPath = Path::normalize(directory);
  auto segments = Utilities::split(currentPath, "/");
  currentPath = Path::isAbsolute(currentPath) ? "" : Process::cwd();
  if (segments[0].empty())
    segments[0] = "/";

  size_t index = 0;
  while (index < segments.size()) {
    currentPath = Path::join({ currentPath, segments[index++] });
    if (currentPath.empty())
      continue;

    if (!isDir(currentPath))
      Platform::get().createFolder(currentPath);
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Checks if the given path is a directory and returns true if so, otherwise false.
 */
bool FS::isDir(std::string const& path) {
#ifdef _MSC_VER
  struct _stat buffer;
  std::wstring converted = Utilities::s2ws(path);
  int result = _wstat(converted.c_str(), &buffer);
  return result == 0 && (buffer.st_mode & _S_IFDIR) != 0;
#else
  struct stat buffer;
  int result = stat(path.c_str(), &buffer);
  return result == 0 && (buffer.st_mode & S_IFDIR) != 0;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Checks if the given path is a normal file and returns true if so, otherwise false.
 */
bool FS::isFile(std::string const& path) {
#ifdef _MSC_VER
  struct _stat buffer;
  std::wstring converted = Utilities::s2ws(path);
  int result = _wstat(converted.c_str(), &buffer);
  return result == 0 && (buffer.st_mode & _S_IFREG) != 0;
#else
  struct stat buffer;
  int result = stat(path.c_str(), &buffer);
  return result == 0 && (buffer.st_mode & S_IFREG) != 0;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Checks if the given path is a symlink and returns true if so, otherwise false.
 */
bool FS::isSymlink(std::string const& path) {
#ifdef _MSC_VER
  std::wstring converted = Utilities::s2ws(path);
  bool ret = false;
  DWORD attributes =  GetFileAttributes(converted.c_str());
  if (attributes != INVALID_FILE_ATTRIBUTES) {
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
      WIN32_FIND_DATA findFileData;
      memset(&findFileData, 0, sizeof(findFileData));
      HANDLE hFind = FindFirstFile(converted.c_str(), &findFileData);
      if (hFind != INVALID_HANDLE_VALUE) {
        if (findFileData.dwReserved0 & IO_REPARSE_TAG_SYMLINK)
          ret = true;
        FindClose(hFind);
      }
    }
  }
  return ret;
#else
  struct stat buffer;
  int result = stat(path.c_str(), &buffer);
  return result == 0 && (buffer.st_mode & S_IFLNK) != 0;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void FS::move(std::string const& source, std::string const& target, MoveOptions options) {
  if (pathExists(target)) {
    if (!options.overwrite)
      return;

    remove(target);
  }

#ifdef _MSC_VER
  std::wstring convertedSource = Utilities::s2ws(source);
  std::wstring convertedTarget = Utilities::s2ws(target);
  if (_wrename(convertedSource.c_str(), convertedTarget.c_str()) != 0) {
    wchar_t *error = _wcserror(errno);
    std::string message = "Error while moving '" + source + "': ";
    throw std::runtime_error(message + Utilities::ws2s(error));
  }
#else
  if (::rename(source.c_str(), target.c_str()) != 0)
    throw std::runtime_error("Error while moving '" + source + "': " + Utilities::getLastError());
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void FS::outputFile(std::string const& fileName, std::string const& content, CreateOptions createOptions) {
  std::ios_base::openmode mode = std::ios::binary;
  if (createOptions.append)
    mode |= std::ios::app;

  if (createOptions.truncate)
    mode |= std::ios::trunc;

  if (createOptions.failOnExist) {
    if (FS::pathExists(fileName))
      throw std::runtime_error("File already exists");
  }

#ifdef _MSC_VER
  std::ofstream output(Utilities::s2ws(fileName), mode);
#else
  std::ofstream output(fileName, mode);
#endif

  if (output.fail())
    throw std::runtime_error("Error while opening target file '" + fileName + "': " + Utilities::getLastError());

  output << content;
  output.close();
  if (output.bad())
    throw std::runtime_error("Error while writing target file '" + fileName + "'" + Utilities::getLastError());
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Tests if the given path (a file or folder) exists.
 */
bool FS::pathExists(std::string const& path) {
#ifdef _MSC_VER
  struct _stat buffer;
  std::wstring converted = Utilities::s2ws(path);
  int result = _wstat(converted.c_str(), &buffer);
#else
  struct stat buffer;
  int result = stat(path.c_str(), &buffer);
#endif

  return result == 0;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Removes a file or directory. The directory can have contents.
 */
void FS::remove(std::string const& path) {
  if (!pathExists(path))
    return;
  
  if (isDir(path)) {
    for (auto &entry : FS::readDir(path))
      remove(Path::join({ path, entry }));

    Platform::get().removeFolder(path);
  } else
    Platform::get().removeFile(path);
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> FS::readDir(std::string const& directory) {
  std::vector<std::string> result;

  // TODO: move to platform layer.
#ifdef _MSC_VER
  WIN32_FIND_DATA data;
  HANDLE hFind = FindFirstFile(Utilities::s2ws(directory + "/*.*").c_str(), &data);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      std::wstring name = data.cFileName;
      if (name != L"." && name != L"..") {
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) == 0) {
          result.push_back(Utilities::ws2s(name));
        }
      }
    } while (FindNextFile(hFind, &data));
    FindClose(hFind);
  }
#elif defined(__APPLE__)
  DIR *handle = opendir(directory.c_str());
  if (handle == nullptr)
    throw std::runtime_error("Error while enumerating folder '" + directory + "': " + Utilities::getLastError());

  dirent *entry;
  while ((entry = readdir(handle)) != nullptr) {
    std::string name(entry->d_name, entry->d_namlen);
    if (name == "." || name == "..")
      continue;

    // According to READDIR(3) applications should handle also the DT_UNKNOWN type.
    if (entry->d_type == DT_DIR || entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN)
      result.push_back(name);
  }
  (void)closedir(handle);
#else
  DIR *handle = opendir(directory.c_str());
  if (handle == nullptr)
    throw std::runtime_error("Error while enumerating folder '" + directory + "': " + Utilities::getLastError());

  dirent *entry;
  while ((entry = readdir(handle)) != nullptr) {
    std::string name(entry->d_name);
    if (name == "." || name == "..")
      continue;

    // According to READDIR(3) applications should handle also the DT_UNKNOWN type.
    if (entry->d_type == DT_DIR || entry->d_type == DT_REG || entry->d_type == DT_UNKNOWN)
      result.push_back(name);
  }
  (void)closedir(handle);
#endif

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void FS::chmod(std::string const& path, int mode) {
#ifdef _MSC_VER
  int retval = ::_chmod(path.c_str(), mode);
#else
  int retval = ::chmod(path.c_str(), mode);
#endif
  if (retval == -1)
    throw std::runtime_error("Error setting file mode for '" + path + "': " + Utilities::getLastError());
}

//----------------------------------------------------------------------------------------------------------------------

void FS::activate(ScriptingContext &context, JSObject &exports) {
  Stats::registerInContext(context, exports);

  exports.defineFunction({ "copySync" }, 3, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    std::string source = args.get(0);
    std::string target = args.get(1);

    CopyOptions copyOptions;
    if (args.is(ValueType::Object, 2)) {
      JSObject options = args.get(2);
      copyOptions.overwrite = options.get("overwrite", false);
      copyOptions.errorOnExist = options.get("errorOnExist", false);
      copyOptions.dereference = options.get("dereference", false);
      copyOptions.preserveTimestamps = options.get("preserveTimestamps", false);

      // TODO: filter function.
    }

    try {
      FS::copy(source, target, copyOptions);
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });

  exports.defineFunction({ "emptyDirSync" }, 1, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    try {
      FS::emptyDir(args.get(0));
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });

  exports.defineFunction({ "ensureDirSync", "mkdirsSync" }, 1, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    try {
      FS::ensureDir(args.get(0));
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });

  exports.defineFunction({ "moveSync", "renameSync" }, 3, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    std::string source = args.get(0);
    std::string target = args.get(1);

    MoveOptions moveOptions;
    if (args.is(ValueType::Object, 2)) {
      JSObject options = args.get(2);
      moveOptions.overwrite = options.get("overwrite", false);
    }

    try {
      FS::move(source, target, moveOptions);
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });

  exports.defineFunction({ "outputFileSync", "writeFileSync" }, 3, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    std::string fileName = args.get(0);
    std::string content = args.get(1);

    CreateOptions createOpts;
    if (args.is(ValueType::String, 2)) {
      std::string opts = args.get(2);
      if (opts == "a") {
        createOpts.append = true;
        createOpts.truncate = false;
      } else if (opts == "ax") {
        createOpts.append = true;
        createOpts.failOnExist = true;
        createOpts.truncate = false;
      } else if (opts == "w") {
        createOpts.append = false;
        createOpts.truncate = true;
      } else if (opts == "wx") {
        createOpts.append = false;
        createOpts.failOnExist = true;
        createOpts.truncate = true;
      }
    }

    try {
      FS::outputFile(fileName, content, createOpts);
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });

  exports.defineFunction({ "pathExistsSync", "existsSync" }, 1, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    std::string path = args.get(0);

    try {
      args.pushResult(FS::pathExists(path));
    } catch (std::runtime_error &) {
      args.pushResult(false);
    }
  });

  exports.defineFunction({ "readdirSync", "readFolderSync" }, 1, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    std::string path = args.get(0);

    try {
      JSArray result(args.context());
      for (auto &entry : FS::readDir(path)) {
        result.addValue(entry);
      }
      args.pushResult(result);
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });

  exports.defineFunction({ "readFileSync" }, 1, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    std::string fileName = args.get(0);

    try {
      args.pushResult(FS::contentFromFile(fileName));
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });

  exports.defineFunction({ "removeSync" }, 1, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    std::string path = args.get(0);

    try {
      FS::remove(path);
    } catch (std::runtime_error &e) {
      args.context()->throwScriptingError(ScriptingError::Error, e.what());
    }
  });

  exports.defineFunction({ "statSync" }, 2, [](JSExport *, JSValues &args) {
    JSObject result = args.context()->createJsInstance("Stats", { args.get(0), true });
    args.pushResult(result);
  });

  exports.defineFunction({ "lstatSync" }, 2, [](JSExport *, JSValues &args) {
    JSObject result = args.context()->createJsInstance("Stats", { args.get(0), false });
    args.pushResult(result);
  });

  exports.defineFunction({ "chmodSync" }, 2, [](JSExport *instance, JSValues &args) {
    std::ignore = instance;
    int mode = args.get(1);
    std::string path = args.get(0);
    FS::chmod(path, mode);
  });

  JSObject constants(&context);
  Platform::get().defineFsConstants(context, constants);
  exports.defineProperty("constants", constants);
}

//----------------------------------------------------------------------------------------------------------------------

bool FS::_registered  = []() {
  ScriptingContext::registerModule("fs", &FS::activate);
  return true;
}();
