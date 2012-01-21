// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/settings/weak_unlimited_settings_storage.h"

namespace extensions {

WeakUnlimitedSettingsStorage::WeakUnlimitedSettingsStorage(
    SettingsStorage* delegate)
    : delegate_(delegate) {}

WeakUnlimitedSettingsStorage::~WeakUnlimitedSettingsStorage() {}

SettingsStorage::ReadResult WeakUnlimitedSettingsStorage::Get(
    const std::string& key) {
  return delegate_->Get(key);
}

SettingsStorage::ReadResult WeakUnlimitedSettingsStorage::Get(
    const std::vector<std::string>& keys) {
  return delegate_->Get(keys);
}

SettingsStorage::ReadResult WeakUnlimitedSettingsStorage::Get() {
  return delegate_->Get();
}

SettingsStorage::WriteResult WeakUnlimitedSettingsStorage::Set(
    WriteOptions options, const std::string& key, const Value& value) {
  return delegate_->Set(IGNORE_QUOTA, key, value);
}

SettingsStorage::WriteResult WeakUnlimitedSettingsStorage::Set(
    WriteOptions options, const DictionaryValue& values) {
  return delegate_->Set(IGNORE_QUOTA, values);
}

SettingsStorage::WriteResult WeakUnlimitedSettingsStorage::Remove(
    const std::string& key) {
  return delegate_->Remove(key);
}

SettingsStorage::WriteResult WeakUnlimitedSettingsStorage::Remove(
    const std::vector<std::string>& keys) {
  return delegate_->Remove(keys);
}

SettingsStorage::WriteResult WeakUnlimitedSettingsStorage::Clear() {
  return delegate_->Clear();
}

}  // namespace extensions
