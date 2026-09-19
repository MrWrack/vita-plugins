# v0.12 Download UI states

Plugin Browser must display:
- CONNECTING...
- DOWNLOADING... 0–100%
- VERIFYING SIZE...
- VERIFYING SHA-256...
- VERIFYING PACKAGE...
- VERIFIED - READY TO INSTALL
- VERIFICATION FAILED / DOWNLOAD FAILED

The install action remains unavailable until `DS_READY_TO_INSTALL`.
A failed download or verification removes the temporary file.
Newly installed plugins remain DISABLED and go to TEST config only.

## Important
The progress/status state machine and VitaSDK HTTP integration boundary are now in source.
The HTTP transport is intentionally disabled/fail-closed until its request lifecycle is compiled
against the actual target VitaSDK and tested on a PS Vita. This source does not falsely report
network success.
