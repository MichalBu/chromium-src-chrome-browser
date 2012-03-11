// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file exists to share common overlay behaviors.

cr.define('cr.ui.overlay', function() {

  /**
   * Gets the top, visible overlay. It makes the assumption that if multiple
   * overlays are visible, the last in the byte order is topmost.
   * TODO(estade): rely on aria-visibility instead?
   * @return {HTMLElement} The overlay.
   */
  function getTopOverlay() {
    var overlays = document.querySelectorAll('.overlay:not([hidden])');
    return overlays[overlays.length - 1];
  }

  /**
   * Makes initializations which must hook at the document level.
   */
  function globalInitialization() {
    /* Listen to focus events and make sure focus doesn't move outside of a
     * visible overlay .page. */
    document.addEventListener('focus', function(e) {
      var overlay = getTopOverlay();
      var page = overlay ? overlay.querySelector('.page:not([hidden])') : null;
      if (!page || page.contains(e.target))
        return;

      var focusElement = page.querySelector('button, input, list, select, a');
      if (focusElement)
        focusElement.focus();
    }, true);

    /* Close the overlay on escape. */
    document.addEventListener('keydown', function(e) {
      if (e.keyCode == 27) {  // Escape
        var overlay = getTopOverlay();
        if (!overlay)
          return;

        cr.dispatchSimpleEvent(overlay, 'cancelOverlay');
      }
    });
  }

  /**
   * Adds behavioral hooks for the given overlay.
   * @param {HTMLElement} overlay The .overlay.
   */
  function setupOverlay(overlay) {
    /* Remove the 'shake' animation any time the overlay is hidden or shown.
     */
    overlay.__defineSetter__('hidden', function(value) {
      this.classList.remove('shake');
      if (value)
        this.setAttribute('hidden', true);
      else
        this.removeAttribute('hidden');
    });
    overlay.__defineGetter__('hidden', function() {
      return this.hasAttribute('hidden');
    });

    /* Shake when the user clicks away. */
    overlay.addEventListener('click', function(e) {
      // Only shake if the overlay was the target of the click.
      if (this != e.target)
        return;

      // This may be null while the overlay is closing.
      var overlayPage = this.querySelector('.page:not([hidden])');
      if (overlayPage)
        overlayPage.classList.add('shake');
    });
    overlay.addEventListener('webkitAnimationEnd', function(e) {
      e.target.classList.remove('shake');
    });
  }

  return {
    globalInitialization: globalInitialization,
    setupOverlay: setupOverlay,
  }
});

document.addEventListener('DOMContentLoaded',
                          cr.ui.overlay.globalInitialization);
