// ICResourceManager.h - resource manager interface: creates, finds, reference-counts and
// destroys CResource objects by handle or name. One instance per application (created by the
// game's CAppFactory). (created by b011x; the iterator interface was drafted by b000x)
// plat 2026-11-10: CK 1.5.2 has no separate interface class - the whole API lives in
// CResourceManager (CResourceManager.h). This header only keeps the CKZ spelling compiling for
// the ported callers.
#ifndef ICRESOURCEMANAGER_H
#define ICRESOURCEMANAGER_H

#include "CResourceManager.h"

/** CKZ name of the resource manager base class. */
typedef CResourceManager ICResourceManager;

#endif
