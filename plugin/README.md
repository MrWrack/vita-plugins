# Vita AutoPlugin v0.22 background-plugin scaffold

The VPK UI cannot remain resident over a game by itself. This folder defines the split architecture for a future taiHEN user/kernel plugin that will own global hotkeys and the in-game overlay.

v0.22 does **not** claim a working global overlay yet. Do not add it to taiHEN config until a compiled plugin module with safe process/input hooks has been implemented and tested on real hardware.

Planned contract:
- persisted settings: `ux0:data/VitaAutoPlugin/settings.cfg`
- `L + SELECT`: Trophy Quick Menu only when Trophy Hunter + Trophy Unlocker are ON
- `R + UP`: normal Quick Menu
- Circle closes overlay only
- no trophy database writes without title/database validation and a verified backup
