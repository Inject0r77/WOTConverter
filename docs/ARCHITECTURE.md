# Architecture

## Goals

The rewrite intentionally prevents UI state from becoming business logic. The original script mixed widgets, localization, rates, validation, conversion formulas, and package selection in one class. The native rewrite separates those responsibilities.

## Layers

### `core`

Pure C++20 with no Win32 dependency.

- `Types.h` — region/source/result data structures.
- `Rates.*` — default rates plus optional `rates.json` loading and validation.
- `Converter.*` — resource conversion engine.
- `BoxOptimizer.*` — exact minimum-cost package selection.

The core can be unit-tested on Linux or Windows without creating a GUI.

### `ui`

Windows-only presentation layer.

- Native top-level Win32 window.
- GDI+ custom painting for cards, sidebar, pills, borders, typography.
- Real Win32 `EDIT` controls for text input instead of hand-rolled keyboard handling.
- PerMonitorV2 DPI scaling.

The UI is a consumer of `Converter` / `BoxOptimizer`; it does not contain conversion formulas.

### `storage`

Small persistence layer for user preferences. It currently stores region, language and modifier state under `%APPDATA%\Inject0r\WoTConverter`.

### `config`

`rates.json` is deployment data, not application state. A malformed/missing file does not make the app unusable because the core has validated built-in defaults.

## Conversion state model

One field is always the last user-edited source (`Money`, `Gold`, `Credits`, or `FreeXp`). An `EN_CHANGE` event for that field creates a `ConversionInput`, the core produces a `ConversionResult`, and the UI updates only the *other* fields while suppressing recursive edit notifications.

That avoids the old behavior where editing one field physically deleted the other inputs.

## Box optimizer

WG package selection is solved by dynamic programming. The search covers the requested amount plus one largest package, which is sufficient to consider a cheaper overshoot without needing an unbounded state space. The result includes both total price and the reconstructed package combination.

## Failure model

- Invalid live input: no crash; result is temporarily not updated.
- Missing/broken `rates.json`: use built-in defaults.
- Invalid individual positive rate: replace that field with its built-in default.
- User settings missing: use default LESTA/RU/off/off state.
