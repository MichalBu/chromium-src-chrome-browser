// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/gdata/gdata_util.h"

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "chrome/common/libxml_utils.h"
#include "chrome/browser/download/download_util.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"

using content::DownloadItem;
using content::DownloadManager;

namespace gdata {
namespace util {

namespace {

const char kGDataMountPointPath[] = "/special/gdata";

const FilePath::CharType kGDataDownloadPath[] = ".gdata";

const FilePath::CharType* kGDataMountPointPathComponents[] = {
  "/", "special", "gdata"
};

}  // namespace

const FilePath& GetGDataMountPointPath() {
  CR_DEFINE_STATIC_LOCAL(FilePath, gdata_mount_path,
      (FilePath::FromUTF8Unsafe(kGDataMountPointPath)));
  return gdata_mount_path;
}

const std::string& GetGDataMountPointPathAsString() {
  CR_DEFINE_STATIC_LOCAL(std::string, gdata_mount_path_string,
      (kGDataMountPointPath));
  return gdata_mount_path_string;
}

bool IsUnderGDataMountPoint(const FilePath& path) {
  return GetGDataMountPointPath() == path ||
         GetGDataMountPointPath().IsParent(path);
}

FilePath ExtractGDataPath(const FilePath& path) {
  if (!IsUnderGDataMountPoint(path))
    return FilePath();

  std::vector<FilePath::StringType> components;
  path.GetComponents(&components);

  // -1 to include 'gdata'.
  FilePath extracted;
  for (size_t i = arraysize(kGDataMountPointPathComponents) - 1;
       i < components.size(); ++i) {
    extracted = extracted.Append(components[i]);
  }
  return extracted;
}

FilePath GetGDataTempDownloadFolderPath() {
  return download_util::GetDefaultDownloadDirectory().Append(
      kGDataDownloadPath);
}
void ParseCreatedResponseContent(const std::string& response_content,
    std::string* resource_id, std::string* md5_checksum) {
  if (response_content.empty())
    return;

  XmlReader xml_reader;
  bool ok = xml_reader.Load(response_content);
  if (!ok) {
    NOTREACHED() << "Invalid xml received " << response_content;
    return;
  }

  // Read the 'entry' node, and then the first node under entry.
  for (int i = 0; i < 2; ++i) {
    ok = xml_reader.Read();
    if (!ok) {
      NOTREACHED() << "Read failed";
      return;
    }
  }

  // Look for nodes for resourceId and md5Checksum.
  while (xml_reader.Next()) {
    if (xml_reader.NodeName() == "resourceId")
      xml_reader.ReadElementContent(resource_id);
    if (xml_reader.NodeName() == "md5Checksum")
      xml_reader.ReadElementContent(md5_checksum);
  }
}


}  // namespace util
}  // namespace gdata
