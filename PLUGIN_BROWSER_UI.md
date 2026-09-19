# Plugin Browser UI — v0.13

Native target: 960×544.

Layout:
- Left: plugin list and selected-row highlight.
- Right: Name, Version, Developer, Category, Description, taiHEN section and status.
- Bottom action area: DOWNLOAD / INSTALL.
- INSTALL is disabled until verification state is `READY TO INSTALL`.
- Status line shows CONNECTING, DOWNLOADING %, VERIFYING SIZE, VERIFYING SHA-256,
  VERIFYING PACKAGE, VERIFIED - READY TO INSTALL, or failure.
- Footer: `↑↓←→ Navigate   ✕ Select   ○ Back   Touch Supported`

Controls:
- D-pad Up/Down: plugin list.
- Left/Right: move between actions/panels.
- X: activate focused action.
- Circle: Back.
- Touch: action hit zones and list selection (list row binding is completed when renderer is wired).

Safety:
- INSTALL cannot be selected before verification.
- Download/verification failures never unlock INSTALL.
- Newly installed plugins are DISABLED and must be explicitly enabled through TEST config.
- Save as Working Config remains manual.
