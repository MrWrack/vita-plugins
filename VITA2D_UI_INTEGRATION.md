# v0.14 Vita2D renderer

Plugin Browser now has a native 960×544 Vita2D renderer.

Important correction:
- Text rendering requires a real `vita2d_pgf *` loaded with `vita2d_load_default_pgf()`.
- The renderer never calls `vita2d_pgf_draw_text(NULL, ...)`.
- The application must load the font once at startup, pass it to render functions, and free it on shutdown.

Screen:
- Header / MrWrack branding
- Plugin list panel
- Details panel
- Verification status
- Download progress bar
- DOWNLOAD button
- INSTALL / INSTALL LOCKED button
- BACK button
- control footer

INSTALL remains visually and logically locked until `DS_READY_TO_INSTALL`.
