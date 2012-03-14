// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_factory.h"

#include "base/command_line.h"
#include "base/environment.h"
#include "chrome/browser/password_manager/login_database.h"
#include "chrome/browser/password_manager/password_store_default.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile_dependency_manager.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"

#if defined(OS_WIN)
#include "chrome/browser/password_manager/password_store_win.h"
#elif defined(OS_MACOSX)
#include "chrome/browser/keychain_mac.h"
#include "chrome/browser/mock_keychain_mac.h"
#include "chrome/browser/password_manager/password_store_mac.h"
#elif defined(OS_CHROMEOS) || defined(OS_ANDROID)
// Don't do anything. We're going to use the default store.
#elif defined(OS_POSIX)
#include "base/nix/xdg_util.h"
#if defined(USE_GNOME_KEYRING)
#include "chrome/browser/password_manager/native_backend_gnome_x.h"
#endif
#include "chrome/browser/password_manager/native_backend_kwallet_x.h"
#include "chrome/browser/password_manager/password_store_x.h"
#endif

#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS) && !defined(OS_ANDROID) && \
    defined(OS_POSIX)
namespace {

const LocalProfileId kInvalidLocalProfileId =
    static_cast<LocalProfileId>(0);

}  // namespace
#endif

PasswordStore* PasswordStoreFactory::GetForProfile(
    Profile* profile,
    Profile::ServiceAccessType sat) {
  if (sat == Profile::IMPLICIT_ACCESS && profile->IsOffTheRecord()) {
    NOTREACHED() << "This profile is OffTheRecord";
    return NULL;
  }

  return static_cast<PasswordStore*>(GetInstance()->GetBaseForProfile(
      profile, true));
}

// static
PasswordStoreFactory* PasswordStoreFactory::GetInstance() {
  return Singleton<PasswordStoreFactory>::get();
}

PasswordStoreFactory::PasswordStoreFactory()
    : RefcountedProfileKeyedServiceFactory(
        "PasswordStore",
        ProfileDependencyManager::GetInstance()) {
  // TODO(erg): We must always depend on WebDB; we don't want the dependency
  // graph to be different based on platform.
  //
  // DependsOn(WebDataServiceFactory::GetInstance());
}

PasswordStoreFactory::~PasswordStoreFactory() {}

#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS) && !defined(OS_ANDROID) && \
    defined(OS_POSIX)
LocalProfileId PasswordStoreFactory::GetLocalProfileId(
    PrefService* prefs) const {
  LocalProfileId id = prefs->GetInteger(prefs::kLocalProfileId);
  if (id == kInvalidLocalProfileId) {
    // Note that there are many more users than this. Thus, by design, this is
    // not a unique id. However, it is large enough that it is very unlikely
    // that it would be repeated twice on a single machine. It is still possible
    // for that to occur though, so the potential results of it actually
    // happening should be considered when using this value.
    static const LocalProfileId kLocalProfileIdMask =
        static_cast<LocalProfileId>((1 << 24) - 1);
    do {
      id = rand() & kLocalProfileIdMask;
      // TODO(mdm): scan other profiles to make sure they are not using this id?
    } while (id == kInvalidLocalProfileId);
    prefs->SetInteger(prefs::kLocalProfileId, id);
  }
  return id;
}
#endif

RefcountedProfileKeyedService* PasswordStoreFactory::BuildServiceInstanceFor(
    Profile* profile) const {
  scoped_refptr<PasswordStore> ps;
  FilePath login_db_file_path = profile->GetPath();
  login_db_file_path = login_db_file_path.Append(chrome::kLoginDataFileName);
  LoginDatabase* login_db = new LoginDatabase();
  if (!login_db->Init(login_db_file_path)) {
    LOG(ERROR) << "Could not initialize login database.";
    delete login_db;
    return NULL;
  }
#if defined(OS_WIN)
  ps = new PasswordStoreWin(
      login_db, profile,
      profile->GetWebDataService(Profile::IMPLICIT_ACCESS));
#elif defined(OS_MACOSX)
  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kUseMockKeychain)) {
    ps = new PasswordStoreMac(new MockKeychain(), login_db);
  } else {
    ps = new PasswordStoreMac(new MacKeychain(), login_db);
  }
#elif defined(OS_CHROMEOS) || defined(OS_ANDROID)
  // For now, we use PasswordStoreDefault. We might want to make a native
  // backend for PasswordStoreX (see below) in the future though.
  ps = new PasswordStoreDefault(
      login_db, profile, profile->GetWebDataService(Profile::IMPLICIT_ACCESS));
#elif defined(OS_POSIX)
  // On POSIX systems, we try to use the "native" password management system of
  // the desktop environment currently running, allowing GNOME Keyring in XFCE.
  // (In all cases we fall back on the basic store in case of failure.)
  base::nix::DesktopEnvironment desktop_env;
  std::string store_type =
      CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kPasswordStore);
  if (store_type == "kwallet") {
    desktop_env = base::nix::DESKTOP_ENVIRONMENT_KDE4;
  } else if (store_type == "gnome") {
    desktop_env = base::nix::DESKTOP_ENVIRONMENT_GNOME;
  } else if (store_type == "basic") {
    desktop_env = base::nix::DESKTOP_ENVIRONMENT_OTHER;
  } else {
    // Detect the store to use automatically.
    scoped_ptr<base::Environment> env(base::Environment::Create());
    desktop_env = base::nix::GetDesktopEnvironment(env.get());
    const char* name = base::nix::GetDesktopEnvironmentName(desktop_env);
    VLOG(1) << "Password storage detected desktop environment: "
            << (name ? name : "(unknown)");
  }

  PrefService* prefs = profile->GetPrefs();
  LocalProfileId id = GetLocalProfileId(prefs);

  scoped_ptr<PasswordStoreX::NativeBackend> backend;
  if (desktop_env == base::nix::DESKTOP_ENVIRONMENT_KDE4) {
    // KDE3 didn't use DBus, which our KWallet store uses.
    VLOG(1) << "Trying KWallet for password storage.";
    backend.reset(new NativeBackendKWallet(id, prefs));
    if (backend->Init())
      VLOG(1) << "Using KWallet for password storage.";
    else
      backend.reset();
  } else if (desktop_env == base::nix::DESKTOP_ENVIRONMENT_GNOME ||
             desktop_env == base::nix::DESKTOP_ENVIRONMENT_XFCE) {
#if defined(USE_GNOME_KEYRING)
    VLOG(1) << "Trying GNOME keyring for password storage.";
    backend.reset(new NativeBackendGnome(id, prefs));
    if (backend->Init())
      VLOG(1) << "Using GNOME keyring for password storage.";
    else
      backend.reset();
#endif  // defined(USE_GNOME_KEYRING)
  }

  if (!backend.get()) {
    LOG(WARNING) << "Using basic (unencrypted) store for password storage. "
        "See http://code.google.com/p/chromium/wiki/LinuxPasswordStorage for "
        "more information about password storage options.";
  }

  ps = new PasswordStoreX(login_db, profile,
                          profile->GetWebDataService(Profile::IMPLICIT_ACCESS),
                          backend.release());
#else
  NOTIMPLEMENTED();
#endif
  if (!ps)
    delete login_db;

  if (!ps || !ps->Init()) {
    NOTREACHED() << "Could not initialize password manager.";
    return NULL;
  }

  return ps.release();
}

void PasswordStoreFactory::RegisterUserPrefs(PrefService* prefs) {
#if !defined(OS_MACOSX) && !defined(OS_CHROMEOS) && !defined(OS_ANDROID) \
  && defined(OS_POSIX)
  prefs->RegisterIntegerPref(prefs::kLocalProfileId,
                             kInvalidLocalProfileId,
                             PrefService::UNSYNCABLE_PREF);

  // Notice that the preprocessor conditions above are exactly those that will
  // result in using PasswordStoreX in CreatePasswordStore() below.
  PasswordStoreX::RegisterUserPrefs(prefs);
#endif
}

bool PasswordStoreFactory::ServiceRedirectedInIncognito() {
  return true;
}

bool PasswordStoreFactory::ServiceIsNULLWhileTesting() {
  return true;
}
