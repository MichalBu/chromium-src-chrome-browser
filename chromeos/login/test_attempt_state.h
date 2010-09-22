// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_TEST_ATTEMPT_STATE_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_TEST_ATTEMPT_STATE_H_
#pragma once

#include <string>

#include "chrome/browser/chromeos/login/auth_attempt_state.h"
#include "chrome/browser/chromeos/login/login_status_consumer.h"
#include "chrome/common/net/gaia/gaia_auth_consumer.h"

namespace chromeos {

class TestAttemptState : public AuthAttemptState {
 public:
  TestAttemptState(const std::string& username,
                   const std::string& password,
                   const std::string& ascii_hash,
                   const std::string& login_token,
                   const std::string& login_captcha);

  virtual ~TestAttemptState();

  // Act as though an online login attempt completed already.
  void PresetOnlineLoginStatus(
      const GaiaAuthConsumer::ClientLoginResult& credentials,
      const LoginFailure& outcome);

  // Act as though an offline login attempt completed already.
  void PresetOfflineLoginStatus(bool offline_outcome, int offline_code);

  // To allow state to be queried on the main thread during tests.
  bool online_complete() { return online_complete_; }
  const LoginFailure& online_outcome() { return online_outcome_; }
  const GaiaAuthConsumer::ClientLoginResult& credentials() {
    return credentials_;
  }
  bool offline_complete() { return offline_complete_; }
  bool offline_outcome() { return offline_outcome_; }
  int offline_code() { return offline_code_; }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestAttemptState);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_TEST_ATTEMPT_STATE_H_
