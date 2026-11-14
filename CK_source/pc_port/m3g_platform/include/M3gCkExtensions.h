/* CK: m3gcore extensions for Swerve semantics, implemented in M3gCkCore.c. */
#ifndef CK_M3G_EXTENSIONS_H
#define CK_M3G_EXTENSIONS_H

#include <m3g_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Group.removeChild as Swerve does it: also removes nodes m3gcore flags as skeleton bones. */
M3G_API void ckM3gRemoveChild(M3GGroup group, M3GNode child);

/** The triangle list of an index buffer with Swerve's corner order (m3gGetBatchSize entries). */
M3G_API void ckM3gGetSwerveTriangleIndices(M3GIndexBuffer buffer, M3Gint *indices);

#ifdef __cplusplus
}
#endif

#endif
