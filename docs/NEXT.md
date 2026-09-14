# Next passes

The current repository is the first working rewrite pass. Planned hardening / polish passes:

1. Compile and smoke-test the Win32 target with MSVC on Windows.
2. Fix any Windows-SDK-specific warnings from `/W4 /permissive-`.
3. Add hover/pressed transitions and keyboard focus visuals to custom-painted controls.
4. Add app icon, VERSIONINFO resource, product metadata and release packaging.
5. Split large page-painting code into page renderers once visual direction is locked.
6. Add a rates editor with validation and an explicit “restore defaults” flow.
7. Add optional signed remote rate manifests only if automatic rate updates are wanted later.
8. Expand unit coverage for boundary values and arbitrary WG package sets.
