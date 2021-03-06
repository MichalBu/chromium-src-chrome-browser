// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILE_MANAGER_VOLUME_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_FILE_MANAGER_VOLUME_MANAGER_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/observer_list.h"
#include "base/prefs/pref_change_registrar.h"
#include "chrome/browser/chromeos/drive/drive_integration_service.h"
#include "chrome/browser/chromeos/file_system_provider/observer.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/local_discovery/storage/privet_volume_lister.h"
#include "chromeos/dbus/cros_disks_client.h"
#include "chromeos/disks/disk_mount_manager.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace chromeos {
class PowerManagerClient;
}  // namespace chromeos

namespace content {
class BrowserContext;
}  // namespace content

namespace drive {
class DriveIntegrationService;
}  // namespace drive

namespace file_manager {

class MountedDiskMonitor;
class VolumeManagerObserver;

// This manager manages "Drive" and "Downloads" in addition to disks managed
// by DiskMountManager.
enum VolumeType {
  VOLUME_TYPE_GOOGLE_DRIVE,
  VOLUME_TYPE_DOWNLOADS_DIRECTORY,
  VOLUME_TYPE_REMOVABLE_DISK_PARTITION,
  VOLUME_TYPE_MOUNTED_ARCHIVE_FILE,
  VOLUME_TYPE_CLOUD_DEVICE,
  VOLUME_TYPE_PROVIDED,  // File system provided by the FileSystemProvider API.
  VOLUME_TYPE_MTP,
  VOLUME_TYPE_TESTING
};

struct VolumeInfo {
  VolumeInfo();
  ~VolumeInfo();

  // The ID for provided file system. If other type, then equal to zero.
  int file_system_id;

  // The ID of the volume.
  std::string volume_id;

  // The type of mounted volume.
  VolumeType type;

  // The type of device. (e.g. USB, SD card, DVD etc.)
  chromeos::DeviceType device_type;

  // The source path of the volume.
  // E.g.:
  // - /home/chronos/user/Downloads/zipfile_path.zip
  base::FilePath source_path;

  // The mount path of the volume.
  // E.g.:
  // - /home/chronos/user/Downloads
  // - /media/removable/usb1
  // - /media/archive/zip1
  base::FilePath mount_path;

  // The mounting condition. See the enum for the details.
  chromeos::disks::MountCondition mount_condition;

  // Path of the system device this device's block is a part of.
  // (e.g. /sys/devices/pci0000:00/.../8:0:0:0/)
  base::FilePath system_path_prefix;

  // If disk is a parent, then its label, else parents label.
  // (e.g. "TransMemory")
  std::string drive_label;

  // Is the device is a parent device (i.e. sdb rather than sdb1).
  bool is_parent;

  // True if the volume is read only.
  bool is_read_only;
};

// Manages "Volume"s for file manager. Here are "Volume"s.
// - Drive File System (not yet supported).
// - Downloads directory.
// - Removable disks (volume will be created for each partition, not only one
//   for a device).
// - Mounted zip archives.
class VolumeManager : public KeyedService,
                      public drive::DriveIntegrationServiceObserver,
                      public chromeos::disks::DiskMountManager::Observer,
                      public chromeos::file_system_provider::Observer {
 public:
  VolumeManager(
      Profile* profile,
      drive::DriveIntegrationService* drive_integration_service,
      chromeos::PowerManagerClient* power_manager_client,
      chromeos::disks::DiskMountManager* disk_mount_manager,
      chromeos::file_system_provider::Service* file_system_provider_service);
  virtual ~VolumeManager();

  // Returns the instance corresponding to the |context|.
  static VolumeManager* Get(content::BrowserContext* context);

  // Intializes this instance.
  void Initialize();

  // Disposes this instance.
  virtual void Shutdown() OVERRIDE;

  // Adds an observer.
  void AddObserver(VolumeManagerObserver* observer);

  // Removes the observer.
  void RemoveObserver(VolumeManagerObserver* observer);

  // Returns the information about all volumes currently mounted.
  std::vector<VolumeInfo> GetVolumeInfoList() const;

  // Finds VolumeInfo for the given volume ID. If found, returns true and the
  // result is written into |result|. Returns false otherwise.
  bool FindVolumeInfoById(const std::string& volume_id,
                          VolumeInfo* result) const;

  // For testing purpose, registers a native local file system poniting to
  // |path| with DOWNLOADS type, and adds its volume info.
  bool RegisterDownloadsDirectoryForTesting(const base::FilePath& path);

  // For testing purpose, adds a volume info pointing to |path|, with TESTING
  // type. Assumes that the mount point is already registered.
  void AddVolumeInfoForTesting(const base::FilePath& path,
                               VolumeType volume_type,
                               chromeos::DeviceType device_type);

  // drive::DriveIntegrationServiceObserver overrides.
  virtual void OnFileSystemMounted() OVERRIDE;
  virtual void OnFileSystemBeingUnmounted() OVERRIDE;

  // chromeos::disks::DiskMountManager::Observer overrides.
  virtual void OnDiskEvent(
      chromeos::disks::DiskMountManager::DiskEvent event,
      const chromeos::disks::DiskMountManager::Disk* disk) OVERRIDE;
  virtual void OnDeviceEvent(
      chromeos::disks::DiskMountManager::DeviceEvent event,
      const std::string& device_path) OVERRIDE;
  virtual void OnMountEvent(
      chromeos::disks::DiskMountManager::MountEvent event,
      chromeos::MountError error_code,
      const chromeos::disks::DiskMountManager::MountPointInfo& mount_info)
      OVERRIDE;
  virtual void OnFormatEvent(
      chromeos::disks::DiskMountManager::FormatEvent event,
      chromeos::FormatError error_code,
      const std::string& device_path) OVERRIDE;

  // chromeos::file_system_provider::Observer overrides.
  virtual void OnProvidedFileSystemMount(
      const chromeos::file_system_provider::ProvidedFileSystem& file_system,
      base::File::Error error) OVERRIDE;
  virtual void OnProvidedFileSystemUnmount(
      const chromeos::file_system_provider::ProvidedFileSystem& file_system,
      base::File::Error error) OVERRIDE;

  // Called on change to kExternalStorageDisabled pref.
  void OnExternalStorageDisabledChanged();

 private:
  void OnPrivetVolumesAvailable(
      const local_discovery::PrivetVolumeLister::VolumeList& volumes);
  void DoMountEvent(chromeos::MountError error_code,
                    const VolumeInfo& volume_info,
                    bool is_remounting);
  void DoUnmountEvent(chromeos::MountError error_code,
                      const VolumeInfo& volume_info);

  Profile* profile_;
  drive::DriveIntegrationService* drive_integration_service_;  // Not owned.
  chromeos::disks::DiskMountManager* disk_mount_manager_;      // Not owned.
  scoped_ptr<MountedDiskMonitor> mounted_disk_monitor_;
  PrefChangeRegistrar pref_change_registrar_;
  ObserverList<VolumeManagerObserver> observers_;
  scoped_ptr<local_discovery::PrivetVolumeLister> privet_volume_lister_;
  chromeos::file_system_provider::Service*
      file_system_provider_service_;  // Not owned by this class.
  std::map<std::string, VolumeInfo> mounted_volumes_;

  DISALLOW_COPY_AND_ASSIGN(VolumeManager);
};

}  // namespace file_manager

#endif  // CHROME_BROWSER_CHROMEOS_FILE_MANAGER_VOLUME_MANAGER_H_
