# Plugin Browser safety flow

1. Refresh repository manifest.
2. Parse metadata.
3. User selects a plugin.
4. Download to a temporary file.
5. Compute SHA-256.
6. Compare against manifest SHA-256.
7. If mismatch: delete temp file, show `VERIFICATION FAILED`, log it, stop.
8. If valid: move plugin to its target plugin folder.
9. Add it to TEST config as **DISABLED**.
10. User must explicitly enable it.
11. User tests the configuration.
12. User manually chooses `Save as Working Config`.

No repository entry can remotely force `default_enabled=true`; the Vita client overrides it to disabled.
Automatic installation/activation is not part of the design.
