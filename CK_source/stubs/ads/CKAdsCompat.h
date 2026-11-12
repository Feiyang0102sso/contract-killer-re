// CKAdsCompat.h - switch for the non-original ad/analytics replacement layers.
//
// The original game links prebuilt AdColony, AdMarvel and Flurry SDKs that the rebuild does not have.
// The rebuild keeps small replacement layers for them; they are NOT original code:
//   - packages/flurry/code/cocoa/native/FlurryAnalytics.m   local session/event log instead of Flurry upload
//   - packages/AdManager/code/cocoa/native/AdMarvelView.m   image ads from an HTTPS gateway (CKAdMarvel*URL)
//   - packages/adcolony/src/AdColony_Facade.mm              initFromBundle: Info.plist config + runtime SDK lookup
//
// CK_ADS_COMPAT 0 (default): the layers do nothing; ad requests fail at once through the original callbacks.
// CK_ADS_COMPAT 1: enable the replacement behaviour (pass -DCK_ADS_COMPAT=1).
// Ad SDK recovery stops here (2026-11-11); Tapjoy and PlayHaven, already rewritten from source, are not affected.
#ifndef CK_ADS_COMPAT_H
#define CK_ADS_COMPAT_H

#ifndef CK_ADS_COMPAT
#define CK_ADS_COMPAT 0
#endif

#endif
