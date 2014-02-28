// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/values.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/extensions/api/api_resource_manager.h"
#include "chrome/browser/extensions/api/socket/socket.h"
#include "chrome/browser/extensions/api/socket/tcp_socket.h"
#include "chrome/browser/extensions/api/sockets_tcp/sockets_tcp_api.h"
#include "chrome/browser/extensions/extension_api_unittest.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace api {

static BrowserContextKeyedService* ApiResourceManagerTestFactory(
    content::BrowserContext* context) {
  content::BrowserThread::ID id;
  CHECK(content::BrowserThread::GetCurrentThreadIdentifier(&id));
  return ApiResourceManager<
      ResumableTCPSocket>::CreateApiResourceManagerForTest(context, id);
}

class SocketsTcpUnitTest : public ExtensionApiUnittest {
 public:
  virtual void SetUp() {
    ExtensionApiUnittest::SetUp();

    ApiResourceManager<ResumableTCPSocket>::GetFactoryInstance()->
        SetTestingFactoryAndUse(browser()->profile(),
                                ApiResourceManagerTestFactory);
  }
};

TEST_F(SocketsTcpUnitTest, Create) {
  // Get BrowserThread
  content::BrowserThread::ID id;
  CHECK(content::BrowserThread::GetCurrentThreadIdentifier(&id));

  // Create SocketCreateFunction and put it on BrowserThread
  SocketsTcpCreateFunction *function = new SocketsTcpCreateFunction();
  function->set_work_thread_id(id);

  // Run tests
  scoped_ptr<base::DictionaryValue> result(RunFunctionAndReturnDictionary(
      function, "[{\"persistent\": true, \"name\": \"foo\"}]"));
  ASSERT_TRUE(result.get());
}

}  // namespace api
}  // namespace extensions
