/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBZIPARCHIVE_ZIPARCHIVE_PRIVATE_H_
#define LIBZIPARCHIVE_ZIPARCHIVE_PRIVATE_H_

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <memory>
#include <vector>

#include <utils/FileMap.h>
#include <ziparchive/zip_archive.h>
#include "android-base/macros.h"

static const char* kErrorMessages[] = {
    "Success",
    "Iteration ended",
    "Zlib error",
    "Invalid file",
    "Invalid handle",
    "Duplicate entries in archive",
    "Empty archive",
    "Entry not found",
    "Invalid offset",
    "Inconsistent information",
    "Invalid entry name",
    "I/O error",
    "File mapping failed",
};

enum ErrorCodes : int32_t {
  kIterationEnd = -1,

  // We encountered a Zlib error when inflating a stream from this file.
  // Usually indicates file corruption.
  kZlibError = -2,

  // The input file cannot be processed as a zip archive. Usually because
  // it's too small, too large or does not have a valid signature.
  kInvalidFile = -3,

  // An invalid iteration / ziparchive handle was passed in as an input
  // argument.
  kInvalidHandle = -4,

  // The zip archive contained two (or possibly more) entries with the same
  // name.
  kDuplicateEntry = -5,

  // The zip archive contains no entries.
  kEmptyArchive = -6,

  // The specified entry was not found in the archive.
  kEntryNotFound = -7,

  // The zip archive contained an invalid local file header pointer.
  kInvalidOffset = -8,

  // The zip archive contained inconsistent entry information. This could
  // be because the central directory & local file header did not agree, or
  // if the actual uncompressed length or crc32 do not match their declared
  // values.
  kInconsistentInformation = -9,

  // An invalid entry name was encountered.
  kInvalidEntryName = -10,

  // An I/O related system call (read, lseek, ftruncate, map) failed.
  kIoError = -11,

  // We were not able to mmap the central directory or entry contents.
  kMmapFailed = -12,

  kLastErrorCode = kMmapFailed,
};

class MappedZipFile {
 public:
  explicit MappedZipFile(const int fd)
      : has_fd_(true), fd_(fd), base_ptr_(nullptr), data_length_(0) {}

  explicit MappedZipFile(void* address, size_t length)
      : has_fd_(false), fd_(-1), base_ptr_(address), data_length_(static_cast<off64_t>(length)) {}

  bool HasFd() const { return has_fd_; }

  int GetFileDescriptor() const;

  void* GetBasePtr() const;

  off64_t GetFileLength() const;

  bool ReadAtOffset(uint8_t* buf, size_t len, off64_t off) const;

 private:
  // If has_fd_ is true, fd is valid and we'll read contents of a zip archive
  // from the file. Otherwise, we're opening the archive from a memory mapped
  // file. In that case, base_ptr_ points to the start of the memory region and
  // data_length_ defines the file length.
  const bool has_fd_;

  const int fd_;

  void* const base_ptr_;
  const off64_t data_length_;
};

class CentralDirectory {
 public:
  CentralDirectory(void) : base_ptr_(nullptr), length_(0) {}

  const uint8_t* GetBasePtr() const { return base_ptr_; }

  size_t GetMapLength() const { return length_; }

  void Initialize(void* map_base_ptr, off64_t cd_start_offset, size_t cd_size);

 private:
  const uint8_t* base_ptr_;
  size_t length_;
};

struct ZipArchive {
  // open Zip archive
  mutable MappedZipFile mapped_zip;
  const bool close_file;

  // mapped central directory area
  off64_t directory_offset;
  CentralDirectory central_directory;
  std::unique_ptr<android_lkchan::FileMap> directory_map;

  // number of entries in the Zip archive
  uint16_t num_entries;

  // We know how many entries are in the Zip archive, so we can have a
  // fixed-size hash table. We define a load factor of 0.75 and over
  // allocate so the maximum number entries can never be higher than
  // ((4 * UINT16_MAX) / 3 + 1) which can safely fit into a uint32_t.
  uint32_t hash_table_size;
  ZipString* hash_table;

  ZipArchive(const int fd, bool assume_ownership)
      : mapped_zip(fd),
        close_file(assume_ownership),
        directory_offset(0),
        central_directory(),
        directory_map(new android_lkchan::FileMap()),
        num_entries(0),
        hash_table_size(0),
        hash_table(nullptr) {}

  ZipArchive(void* address, size_t length)
      : mapped_zip(address, length),
        close_file(false),
        directory_offset(0),
        central_directory(),
        directory_map(new android_lkchan::FileMap()),
        num_entries(0),
        hash_table_size(0),
        hash_table(nullptr) {}

  ~ZipArchive() {
    if (close_file && mapped_zip.GetFileDescriptor() >= 0) {
      close(mapped_zip.GetFileDescriptor());
    }

    free(hash_table);
  }

  bool InitializeCentralDirectory(const char* debug_file_name, off64_t cd_start_offset,
                                  size_t cd_size);
};

#endif  // LIBZIPARCHIVE_ZIPARCHIVE_PRIVATE_H_
