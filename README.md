# Vita AutoPlugin — MrWrack (VPK source milestone)

This is the second VitaSDK source milestone for the VPK.

Implemented now:
- PS Vita 960x544 native UI layout
- readable rendered text (not rasterized menu text)
- MrWrack branding
- D-pad navigation + X toggles
- HUD master ON/OFF
- FPS, CPU Clock, GPU Clock, RAM Usage, Battery %, BAT TEMP individual ON/OFF
- C/F temperature setting; Celsius default
- Auto Save forced OFF
- Auto Backup default ON
- R + D-pad Up overlay state shortcut
- LiveArea files included at explicit package paths
- icon0.png is 128x128 RGBA

Still to implement/test on real Vita:
- actual in-game overlay plugin/hooks and FPS measurement
- live CPU/GPU/RAM/battery readings
- touch hit-testing
- plugin/taiHEN config editor + Last Working recovery
- overclock test profile/recovery
- Trophy Unlock integration
- networking/update/news
- full submenu screens

Build:
mkdir build && cd build
cmake ..
make

Requires VitaSDK plus vita2d. Output target: VitaAutoPlugin.vpk

## v0.16 navigation milestone
- Functional main navigation between Plugin Manager, Trophy Unlocker, System Monitor, Overclock,
  Recovery & Backup, Update Center, News, Settings and About.
- System Monitor toggles are interactive and persisted.
- Celsius/Fahrenheit can be changed with X or left/right.
- Trophy/Overclock/Plugin/Recovery/Update actions are deliberately non-destructive UI placeholders
  until the underlying Vita APIs, validation, backups and recovery logic are implemented and tested.

## v0.16 safety-first controls
- Trophy Hunter / Trophy Unlocker / Auto Platinum / Backup Before Unlock toggles are now interactive.
- Unlock actions remain intentionally blocked until current-title validation, trophy DB verification,
  pre-unlock backup and post-write verification are implemented.
- Overclock UI now stages CPU 444 MHz / GPU 222 MHz as a TEST profile.
- It does NOT yet apply clock writes: rollback and abnormal-start recovery come first.
- Verified VitaSDK power headers expose battery temperature and clock getter/setter APIs; final
  implementation will use real values only, not mock temperatures.

## v0.16 recovery milestone
- TEST overclock profile is persisted separately from Last Working Profile.
- Save as Working Profile promotes the staged profile manually.
- Restore Last Working restores the saved working profile.
- Reset to Default clears the staged test profile.
- Startup recovery detects an unfinished prior session when a TEST profile was staged,
  discards that TEST profile and restores Last Working.
- Recovery events are appended to ux0:data/VitaAutoPlugin/recovery.log.
- Hardware clock writes are still intentionally disabled until this recovery path is
  validated on a real Vita.

## v0.16 taiHEN configuration safety milestone
- Detects ur0:tai/config.txt first, with ux0:tai/config.txt fallback.
- Test Config creates a separate editable snapshot under ux0:data/VitaAutoPlugin.
- Save as Working Config is manual only and stores a Last Working snapshot.
- Manual config backup support added.
- Restore Last Working backs up the current live config before restoring.
- Auto Save remains locked OFF.
- Plugin enable/disable line editing is NOT implemented yet: preserving taiHEN sections,
  ordering and comments correctly comes before modifying config entries.

## v0.16 taiHEN parser milestone
- Added a dedicated tai_config parser/writer module.
- Preserves every original line, including section headers, comments, blank lines and unknown entries.
- Tracks the active section for plugin entries (*KERNEL, *ALL, *main, Title IDs, etc.).
- Recognizes .skprx and .suprx entries without rewriting unrelated content.
- Added reversible TEST-snapshot enable/disable representation using `# MRWRACK_DISABLED`.
- The parser is compiled into the Vita project.
- Live taiHEN config editing is still deliberately disabled: edits will first be applied to
  config.test.txt and validated before any manual promotion/restoration path can touch live config.

## v0.16 Installed Plugins milestone
- Installed Plugins now opens a real list parsed from taiHEN config.
- Shows .skprx/.suprx path, ENABLED/DISABLED state and owning taiHEN section.
- X toggles plugin state in `config.test.txt` only.
- Live `ur0:tai/config.txt` / `ux0:tai/config.txt` is never changed by the toggle screen.
- Circle returns to Plugin Manager.
- Existing comments/order/sections remain preserved by the parser.
- Promotion to a working/live configuration remains a separate manual recovery-controlled step.

## v0.16 Settings / backup discoverability milestone
- Added Settings > Storage & Backup Paths.
- Shows the exact live, fallback, test, working, backup, recovery-log and settings paths.
- Added prominent SAVE WORKING CONFIG and CREATE BACKUP NOW actions inside Settings.
- Backup storage is now consistently under `ux0:data/VitaAutoPlugin/backups/`.
- Added concise PC manual-editing guidance for VitaShell USB/FTP workflows.
- Auto Save remains OFF and locked.
- The UI reminds users to backup, edit/test safely, then manually save a known-good config.

## v0.16 Plugin Browser / repository milestone
- Added repository plugin metadata model and local manifest parser.
- Client forcibly sets every repository plugin to DISABLED by default.
- Added fields for version, developer, category, download URL, SHA-256, taiHEN section and description.
- Added documented install pipeline: temporary download -> SHA-256 verification -> install -> TEST config DISABLED.
- Automatic plugin activation is explicitly prohibited by the client design.
- Networking/download and SHA-256 implementation are NOT claimed complete yet; this milestone builds the safe data layer first.
- All existing Trophy, Overclock, System Monitor, Recovery, Update, News, Settings and About work remains in the tree.

## v0.16 verified-download milestone
- Added a self-contained SHA-256 file implementation and case-insensitive hash verification.
- Added a fail-closed verified download pipeline API.
- Only HTTPS repository URLs are accepted by the pipeline.
- Temporary files are removed on transport or SHA-256 verification failure.
- A failed verification can never proceed to plugin installation.
- New plugins remain DISABLED and must go through TEST config before manual Save as Working Config.
- Vita HTTP transport is deliberately not reported as complete: the current transport stub returns failure until the real Vita networking layer is integrated and tested.

## v0.16 integrity / anti-corruption milestone
- Added expected file-size verification before installation.
- SHA-256 verification remains mandatory.
- Added VPK/ZIP container sanity validation before an install may continue.
- Any size/hash/container failure deletes the temporary download and stops the pipeline.
- Added `windows/verify_package.py` as the verification core for later Windows Repository Manager EXE integration. It calculates SHA-256 + exact byte size and runs ZIP CRC testing before publishing/copying a VPK.
- Intended end-to-end rule: Windows verify -> transfer/download -> Vita size verify -> Vita SHA-256 verify -> Vita package sanity check -> install.
- This greatly reduces corrupt/incomplete-package installs, but does not falsely promise that every Vita installation error can be prevented.
- Vita HTTP/HTTPS transport is still fail-closed pending real VitaSDK integration/device testing.

## v0.16 download/status milestone
- Added download progress/status state machine for Plugin Browser.
- Added CONNECTING, DOWNLOADING, VERIFYING SIZE, VERIFYING SHA-256, VERIFYING PACKAGE, READY TO INSTALL and FAILED states.
- Verified pipeline now exposes these states to the UI.
- Added a dedicated VitaSDK HTTP/HTTPS transport module and compile-time integration boundary.
- Transport remains fail-closed by default until compiled with the target VitaSDK and tested on real Vita hardware.
- Install cannot become available until all verification stages pass.
- Existing master menus/features remain part of the project; this milestone does not remove Trophy, Overclock, System Monitor, Recovery, Update Center, News, Logs, Settings or About.

## v0.16 Plugin Browser interaction milestone
- Added a native 960×544 Plugin Browser UI state/controller module.
- D-pad navigation for plugin selection and action focus.
- X activation model for Download / Install / Back.
- Touch hit zones for Download, Install and Back.
- INSTALL remains hard-disabled until the verified-download pipeline reports READY TO INSTALL.
- UI consumes the v0.12 progress/verification states.
- Renderer wiring to the existing Vita2D screen is the next integration step.
- No existing master category was removed: Home, Plugin Browser, Installed Plugins,
  Trophy Unlock, System Monitor, Overclock, Update Center, News, Recovery & Backup,
  Logs, Settings and About remain required.

## v0.16 Vita2D UI rendering milestone
- Added a native 960×544 Vita2D Plugin Browser renderer.
- Added plugin list, details panel, verification status, progress bar, Download, Install and Back controls.
- INSTALL is visibly locked until verification succeeds.
- Fixed the renderer architecture so it requires a real loaded `vita2d_pgf` font handle; it does not use NULL for PGF text drawing.
- Added default PGF font load/free helpers.
- Input state from v0.13 and verification state from v0.12 feed the renderer.
- This is source integration; it has not been claimed as a hardware-tested compiled VPK yet.

## v0.16 input/main-loop milestone
- Added PS Vita controller input for D-pad, X and Circle.
- Added front-touch input mapped to the native 960×544 UI.
- Added a self-contained Plugin Browser Vita2D render/input loop.
- Circle returns from Plugin Browser.
- Download/install actions remain safe integration hooks until real repository/network/install wiring is ready.
- The next step is compilation validation and correcting VitaSDK/API/linker issues found by a real build.

## v0.16 compile-readiness audit
- Corrected the remaining NULL-PGF call in the legacy main UI.
- Main UI now owns a real default PGF font lifecycle.
- Added missing stdio declaration for `snprintf` in Plugin Browser screen code.
- Added ScePgf linker stub.
- CI now retains the compiler/linker build log.
- VitaSDK is not installed in the current execution environment, so no false local compile-success claim is made. GitHub Actions remains the real VitaSDK build check.
