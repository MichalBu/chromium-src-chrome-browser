// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prerender/prerender_observer.h"

#include "base/time.h"
#include "chrome/browser/prerender/prerender_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/view_messages.h"

namespace prerender {

PrerenderObserver::PrerenderObserver(TabContents* tab_contents)
    : TabContentsObserver(tab_contents),
      pplt_load_start_() {
}

PrerenderObserver::~PrerenderObserver() {
}

void PrerenderObserver::ProvisionalChangeToMainFrameUrl(const GURL& url) {
  PrerenderManager* prerender_manager = MaybeGetPrerenderManager();
  if (!prerender_manager)
    return;
  if (prerender_manager->IsTabContentsPrerendering(tab_contents()))
    return;
  prerender_manager->MarkTabContentsAsNotPrerendered(tab_contents());
  MaybeUsePreloadedPage(url);
}

bool PrerenderObserver::OnMessageReceived(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(PrerenderObserver, message)
    IPC_MESSAGE_HANDLER(ViewHostMsg_DidStartProvisionalLoadForFrame,
                        OnDidStartProvisionalLoadForFrame)
  IPC_END_MESSAGE_MAP()
  return false;
}

void PrerenderObserver::OnDidStartProvisionalLoadForFrame(int64 frame_id,
                                                          bool is_main_frame,
                                                          const GURL& url) {
  // Don't include prerendered pages in the PPLT metric until after they are
  // swapped in.
  if (IsPrerendering())
    return;
  if (is_main_frame) {
    // Record the beginning of a new PPLT navigation.
    pplt_load_start_ = base::TimeTicks::Now();
  }
}

void PrerenderObserver::DidStopLoading() {
  // Don't include prerendered pages in the PPLT metric until after they are
  // swapped in.
  if (IsPrerendering())
    return;

  // Compute the PPLT metric and report it in a histogram, if needed.
  if (!pplt_load_start_.is_null()) {
    PrerenderManager::RecordPerceivedPageLoadTime(
        base::TimeTicks::Now() - pplt_load_start_, tab_contents());
  }

  // Reset the PPLT metric.
  pplt_load_start_ = base::TimeTicks();
}

PrerenderManager* PrerenderObserver::MaybeGetPrerenderManager() {
  return tab_contents()->profile()->GetPrerenderManager();
}

bool PrerenderObserver::MaybeUsePreloadedPage(const GURL& url) {
  PrerenderManager* prerender_manager = MaybeGetPrerenderManager();
  if (!prerender_manager)
    return false;
  DCHECK(!prerender_manager->IsTabContentsPrerendering(tab_contents()));
  if (prerender_manager->MaybeUsePreloadedPage(tab_contents(), url))
    return true;
  return false;
}

bool PrerenderObserver::IsPrerendering() {
  PrerenderManager* prerender_manager = MaybeGetPrerenderManager();
  if (!prerender_manager)
    return false;
  return prerender_manager->IsTabContentsPrerendering(tab_contents());
}

}  // namespace prerender
