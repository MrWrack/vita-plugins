# v0.16 compile audit

This environment does not contain VitaSDK (`arm-vita-eabi-gcc` is unavailable), so a truthful local Vita build could not be run here.

Static build-readiness fixes made before CI:
- Fixed the legacy `main.c` renderer that still called `vita2d_pgf_draw_text(NULL, ...)`.
- Main UI now loads one real default PGF at startup, uses it for all text, and frees it on shutdown.
- Added missing `<stdio.h>` to `plugin_browser_screen.c` for `snprintf`.
- Cleaned `repository.h` pragma/include ordering.
- Added `ScePgf_stub` to link libraries for default PGF use.
- GitHub Actions now uploads `build.log` together with the VPK artifact.

Next validation:
1. Push v0.16 to GitHub.
2. Run `Build Vita AutoPlugin`.
3. Inspect the uploaded build log if compilation/linking fails.
4. Fix only the actual VitaSDK errors reported by that build.
5. Do not call the VPK hardware-tested until it is installed and exercised on a real PS Vita.
