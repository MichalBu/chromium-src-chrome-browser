// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/network_profile_bubble.h"

#include <windows.h>

#include <wtsapi32.h>
// Make sure we link the wtsapi lib file in.
#pragma comment(lib, "wtsapi32.lib")

#include "base/bind.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/prefs/pref_service.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "components/user_prefs/pref_registry_syncable.h"
#include "content/public/browser/browser_thread.h"

namespace {

// The duration of the silent period before we start nagging the user again.
const int kSilenceDurationDays = 100;

// The number of warnings to be shown on consequtive starts of Chrome before the
// silent period starts.
const int kMaxWarnings = 2;

// Implementation of chrome::BrowserListObserver used to wait for a browser
// window.
class BrowserListObserver : public chrome::BrowserListObserver {
 private:
  virtual ~BrowserListObserver();

  // Overridden from chrome::BrowserListObserver:
  virtual void OnBrowserAdded(Browser* browser) OVERRIDE;
  virtual void OnBrowserRemoved(Browser* browser) OVERRIDE;
  virtual void OnBrowserSetLastActive(Browser* browser) OVERRIDE;
};

BrowserListObserver::~BrowserListObserver() {
}

void BrowserListObserver::OnBrowserAdded(Browser* browser) {
}

void BrowserListObserver::OnBrowserRemoved(Browser* browser) {
}

void BrowserListObserver::OnBrowserSetLastActive(Browser* browser) {
  NetworkProfileBubble::ShowNotification(browser);
  // No need to observe anymore.
  BrowserList::RemoveObserver(this);
  delete this;
}

// The name of the UMA histogram collecting our stats.
const char kMetricNetworkedProfileCheck[] = "NetworkedProfile.Check";

}  // namespace

// static
bool NetworkProfileBubble::notification_shown_ = false;

// static
bool NetworkProfileBubble::ShouldCheckNetworkProfile(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  if (prefs->GetInteger(prefs::kNetworkProfileWarningsLeft))
    return !notification_shown_;
  int64 last_check = prefs->GetInt64(prefs::kNetworkProfileLastWarningTime);
  base::TimeDelta time_since_last_check =
      base::Time::Now() - base::Time::FromTimeT(last_check);
  if (time_since_last_check.InDays() > kSilenceDurationDays) {
    prefs->SetInteger(prefs::kNetworkProfileWarningsLeft, kMaxWarnings);
    return !notification_shown_;
  }
  return false;
}

// static
void NetworkProfileBubble::CheckNetworkProfile(
    const base::FilePath& profile_folder) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::FILE));
  // On Windows notify the users if their profiles are located on a network
  // share as we don't officially support this setup yet.
  // However we don't want to bother users on Cytrix setups as those have no
  // real choice and their admins must be well aware of the risks associated.
  // Also the command line flag --no-network-profile-warning can stop this
  // warning from popping up. In this case we can skip the check to make the
  // start faster.
  // Collect a lot of stats along the way to see which cases do occur in the
  // wild often enough.
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kNoNetworkProfileWarning)) {
    RecordUmaEvent(METRIC_CHECK_SUPPRESSED);
    return;
  }

  LPWSTR buffer = NULL;
  DWORD buffer_length = 0;
  // Checking for RDP is cheaper than checking for a network drive so do this
  // one first.
  if (!::WTSQuerySessionInformation(WTS_CURRENT_SERVER, WTS_CURRENT_SESSION,
                                    WTSClientProtocolType,
                                    &buffer, &buffer_length)) {
    RecordUmaEvent(METRIC_CHECK_FAILED);
    return;
  }

  unsigned short* type = reinterpret_cast<unsigned short*>(buffer);
  // We should warn the users if they have their profile on a network share only
  // if running on a local session.
  if (*type == WTS_PROTOCOL_TYPE_CONSOLE) {
    bool profile_on_network = false;
    if (!profile_folder.empty()) {
      base::FilePath temp_file;
      // Try to create some non-empty temp file in the profile dir and use
      // it to check if there is a reparse-point free path to it.
      if (file_util::CreateTemporaryFileInDir(profile_folder, &temp_file) &&
          file_util::WriteFile(temp_file, ".", 1)) {
        base::FilePath normalized_temp_file;
        if (!file_util::NormalizeFilePath(temp_file, &normalized_temp_file))
          profile_on_network = true;
      } else {
        RecordUmaEvent(METRIC_CHECK_IO_FAILED);
      }
      base::Delete(temp_file, false);
    }
    if (profile_on_network) {
      RecordUmaEvent(METRIC_PROFILE_ON_NETWORK);
      content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
          base::Bind(&NotifyNetworkProfileDetected));
    } else {
      RecordUmaEvent(METRIC_PROFILE_NOT_ON_NETWORK);
    }
  } else {
    RecordUmaEvent(METRIC_REMOTE_SESSION);
  }

  ::WTSFreeMemory(buffer);
}

// static
void NetworkProfileBubble::SetNotificationShown(bool shown) {
  notification_shown_ = shown;
}

// static
void NetworkProfileBubble::RegisterUserPrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(
      prefs::kNetworkProfileWarningsLeft,
      kMaxWarnings,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
  registry->RegisterInt64Pref(
      prefs::kNetworkProfileLastWarningTime,
      0,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
}

// static
void NetworkProfileBubble::RecordUmaEvent(MetricNetworkedProfileCheck event) {
  UMA_HISTOGRAM_ENUMERATION(kMetricNetworkedProfileCheck,
                            event,
                            METRIC_NETWORKED_PROFILE_CHECK_SIZE);
}

// static
void NetworkProfileBubble::NotifyNetworkProfileDetected() {
  Browser* browser = chrome::FindLastActiveWithHostDesktopType(
      chrome::GetActiveDesktop());

  if (browser)
    ShowNotification(browser);
  else
    BrowserList::AddObserver(new BrowserListObserver());
}
