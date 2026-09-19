# v0.15 main-loop integration

Plugin Browser now has a self-contained Vita2D screen loop.

Input:
- D-pad Up/Down: list navigation
- D-pad Left/Right: action focus
- X: activate focused action
- Circle: leave Plugin Browser
- Front touch: mapped from Vita touch coordinates to 960×544 UI coordinates

Rendering:
- Starts Vita2D drawing
- Clears screen
- Renders Plugin Browser
- Ends drawing / swaps buffers
- ~16 ms loop delay

Safety:
- Download and install actions remain explicit integration hooks.
- No fake network success.
- No install before verified state.
- New plugins remain DISABLED.
