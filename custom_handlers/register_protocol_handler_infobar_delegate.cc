// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/custom_handlers/register_protocol_handler_infobar_delegate.h"

#include "base/utf_string_conversions.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/common/url_constants.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

RegisterProtocolHandlerInfoBarDelegate::RegisterProtocolHandlerInfoBarDelegate(
    TabContents* tab_contents,
    ProtocolHandlerRegistry* registry,
    const ProtocolHandler& handler)
    : ConfirmInfoBarDelegate(tab_contents),
      tab_contents_(tab_contents),
      registry_(registry),
      handler_(handler) {
}

bool RegisterProtocolHandlerInfoBarDelegate::ShouldExpire(
    const content::LoadCommittedDetails& details) const {
  // The user has submitted a form, causing the page to navigate elsewhere. We
  // don't want the infobar to be expired at this point, because the user won't
  // get a chance to answer the question.
  return false;
}

InfoBarDelegate::Type
RegisterProtocolHandlerInfoBarDelegate::GetInfoBarType() const {
  return PAGE_ACTION_TYPE;
}

string16 RegisterProtocolHandlerInfoBarDelegate::GetMessageText() const {
  ProtocolHandler old_handler = registry_->GetHandlerFor(handler_.protocol());
  return !old_handler.IsEmpty() ?
      l10n_util::GetStringFUTF16(IDS_REGISTER_PROTOCOL_HANDLER_CONFIRM_REPLACE,
          handler_.title(), UTF8ToUTF16(handler_.url().host()),
          GetProtocolName(handler_), old_handler.title()) :
      l10n_util::GetStringFUTF16(IDS_REGISTER_PROTOCOL_HANDLER_CONFIRM,
          handler_.title(), UTF8ToUTF16(handler_.url().host()),
          GetProtocolName(handler_));
}

string16 RegisterProtocolHandlerInfoBarDelegate::GetProtocolName(
    const ProtocolHandler& handler) const {
  if (handler.protocol() == "mailto")
    return l10n_util::GetStringUTF16(IDS_REGISTER_PROTOCOL_HANDLER_MAILTO_NAME);
  if (handler.protocol() == "webcal")
    return l10n_util::GetStringUTF16(IDS_REGISTER_PROTOCOL_HANDLER_WEBCAL_NAME);
  return UTF8ToUTF16(handler.protocol());
}

string16 RegisterProtocolHandlerInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return (button == BUTTON_OK) ?
      l10n_util::GetStringFUTF16(IDS_REGISTER_PROTOCOL_HANDLER_ACCEPT,
                                 handler_.title()) :
      l10n_util::GetStringUTF16(IDS_REGISTER_PROTOCOL_HANDLER_DENY);
}

bool RegisterProtocolHandlerInfoBarDelegate::Accept() {
  registry_->OnAcceptRegisterProtocolHandler(handler_);
  return true;
}

bool RegisterProtocolHandlerInfoBarDelegate::Cancel() {
  registry_->OnIgnoreRegisterProtocolHandler(handler_);
  return true;
}

string16 RegisterProtocolHandlerInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

bool RegisterProtocolHandlerInfoBarDelegate::LinkClicked(
    WindowOpenDisposition disposition) {
  // Ignore the click dispostion and always open in a new top level tab.
  tab_contents_->OpenURL(GURL(chrome::kLearnMoreRegisterProtocolHandlerURL),
                         GURL(), NEW_FOREGROUND_TAB, PageTransition::LINK);
  return false;
}
