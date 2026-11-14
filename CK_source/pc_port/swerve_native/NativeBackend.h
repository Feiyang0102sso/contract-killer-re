// Entry point of the native Swerve backend for code that does not include m3gcore headers.
// [graph] Engine in BountyHunter.cfg chooses the backend: "m3g" is the native m3gcore runtime,
// "swerve" the emulated original (Unicorn + swerve-arm.bin), kept for A/B comparison.
#pragma once

struct ISwerve;

namespace ck::native {

/** Create the native runtime root object (swvNew of the native backend). Returns 0 on success. */
int CreateSwerve(ISwerve** swerve);

} // namespace ck::native
