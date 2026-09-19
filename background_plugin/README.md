# Vita AutoPlugin Background Alpha

This is the first background-plugin build stage.

Target behavior:
- load on SceShell/Home and supported apps through taiHEN;
- R + D-pad Up toggles Quick Menu state;
- Circle always closes;
- later overlay: FPS | CPU | GPU | MEM | TEMP in the upper-right.

## Important
This alpha does **not** yet draw the overlay outside the VPK. It establishes
the persistent taiHEN module and input-state logic first. Display/GXM hooks and
verified telemetry must be added and tested on real Vita hardware before they
are marked working.

Suggested taiHEN layout after the .suprx is hardware-tested:

    *main
    ur0:tai/VitaAutoPluginBackground.suprx

For games/apps, loading policy needs compatibility testing before automatically
adding a broad `*ALL` entry.
