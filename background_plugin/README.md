# Vita AutoPlugin Background v0.31

This component is the taiHEN background module. The hotkey is **R + D-pad Up** and is state-only/passive: it never consumes D-pad, face buttons, sticks, or touch.

## Important current limitation

The module in this package does **not yet render the HUD globally**. A controller polling thread alone cannot draw over SceShell/games. Global rendering needs a validated Shell/kernel overlay implementation. The VPK HUD is therefore still app-local in v0.31.

A proven PS Vita design uses a kernel plugin plus a `*main` Shell plugin; PSVshellPlus uses this split for its global GPU HUD, memory and FPS tracking. Vita AutoPlugin will need an equivalent validated renderer before global HUD is marked complete.

## Intended taiHEN layout once renderer is implemented

```text
*KERNEL
ur0:tai/VitaAutoPlugin_Kernel.skprx
*main
ur0:tai/VitaAutoPlugin_Shell.suprx
```

Reboot after changing `ur0:tai/config.txt`.
