// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/chrome_version_service.h"

#include "base/prefs/pref_service.h"
#include "base/version.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/common/pref_names.h"
#include "components/user_prefs/pref_registry_syncable.h"

// static
void ChromeVersionService::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterStringPref(
      prefs::kProfileCreatedByVersion,
      "1.0.0.0",
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
}

// static
void ChromeVersionService::SetVersion(PrefService* prefs,
                                      const std::string& version) {
  prefs->SetString(prefs::kProfileCreatedByVersion, version);
}

// static
std::string ChromeVersionService::GetVersion(PrefService* prefs) {
  return prefs->GetString(prefs::kProfileCreatedByVersion);
}

// static
void ChromeVersionService::OnProfileLoaded(PrefService* prefs,
                                           bool is_new_profile) {
  // Obtain the Chrome version info.
  chrome::VersionInfo version_info;

  // If this is a new profile set version to current version, otherwise
  // (pre-existing profile), leave pref at default value (1.0.0.0) to
  // avoid any first-run behavior.
  std::string version = version_info.Version();
  if (prefs->FindPreference(prefs::kProfileCreatedByVersion)->
      IsDefaultValue() && is_new_profile) {
    SetVersion(prefs, version);
  }
}
