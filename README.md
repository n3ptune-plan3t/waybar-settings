# waybar-settings

A Qt6 Widgets settings manager for [Waybar](https://github.com/Alexays/Waybar).

It edits `~/.config/waybar/config.jsonc` (bar options, module layout and every
per-module option) and writes a generated block into `~/.config/waybar/style.css`.
Comments, key order, formatting and any key the app does not recognise survive a
round trip.

## What it does

**Modules** — a searchable palette of every module Waybar ships (plus templates
for `custom/*`, `custom-graph/*`, `group/*` and `cffi/*`), three live sections
that accept drag and drop from the palette and between each other, and per-module
buttons for reorder, duplicate, rename (`F2`) and remove.

**Module options** — a schema driven editor instead of a key/value table. The
option tables are transcribed from the Waybar sources, so each option gets the
right widget: checkboxes for booleans, enumerated combos with Waybar's own
allowed values, ordered list editors for `paths`/`ignore-workspaces`/font stacks,
a name/threshold table for `states`, a seconds/event-driven/once picker for
`interval`, file pickers for paths, and JSON editors for the free-form objects.
Every field shows its Waybar default; a field left at that default is not written
to the file at all, so the generated config stays as small as the documentation's.
Options the schema does not describe are kept verbatim in a raw JSON box, and the
box is highlighted when the JSON does not parse.

**Styling** — colour roles (background, text, accent, warning, critical, good,
muted, module surface) with 30 presets or per-role custom colours, a font stack,
metrics, per-section underline, bar opacity and an optional raised "pill" look.
Per-module overrides live on the module page next to the module's own options:
background, text and border colours, each one either inherited, taken from a bar
role, borrowed from another preset theme, or a custom colour. Modules that emit
state classes (`battery` → `.charging`, `.full`, …; `network` → `.wifi`,
`.disconnected`, …) get a row per class so a single module's `.critical` can look
different from everyone else's.

**Preview** — a painted bar, not a label: real module order (the right-hand
section is reversed the way Waybar does it), real fonts, real colours, real
radii and padding, plus the underline, the legend of state fills, and the
contrast ratio of the current text. The backdrop switches between dark, mid and
light so a low bar opacity shows its real effect.

**Readability** — every text colour is guaranteed at least 4.5:1 against the bar,
against the module surface, and against a fully black or fully white wallpaper.
When a pick cannot satisfy that the colour is nudged or the opacity is raised, and
the adjustment is listed on the Style tab. Your original picks are what get
written to the file; only the derived palette changes.

## Files touched

- `~/.config/waybar/config.jsonc` (or `config.json` / `config`) — a `.bak` copy is
  written first. The document is edited in place: comments and untouched values
  are copied verbatim. If the file cannot be parsed, saving is disabled.
- `~/.config/waybar/style.css` — only the text between
  `/* waybar-settings:begin */` and `/* waybar-settings:end */`, and only when
  "Manage a generated block" is ticked. Everything you wrote by hand is kept.
  A `.bak` copy is written first.
- `~/.config/waybar/waybar-settings.json` — the colour roles, fonts, metrics and
  per-module style overrides. Created on first save.

"Save and reload Waybar" runs `pkill -USR2 waybar`.

## Build

Requires Qt6 Widgets and CMake.

    cmake -B build && cmake --build build
    ./build/waybar-settings
    sudo cmake --install build

## Layout

    src/jsonc.*        JSONC document that preserves comments and formatting
    src/schema.*       every Waybar module and option, with types and defaults
    src/style.*        colour roles, contrast guarantees, style model, CSS writer
    src/widgets.*      colour source, list/interval/state/JSON editors, option form
    src/preview.*      the painted bar preview
    src/layoutpage.*   palette + sections + module editor
    src/stylepage.*    theme editor and readability report
    src/mainwindow.*   document ownership, load, save
