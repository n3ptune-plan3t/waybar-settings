# waybar-settings

Qt6 Widgets GUI settings manager for [Waybar](https://github.com/Alexays/Waybar) (targets 0.15.0).

## Features

- **Bar:** position, layer, mode, height, width, spacing, margin, output, name, and bool flags.
- **Modules:** drag-and-drop layout of left/center/right modules; add any module name (`custom/foo`, `clock#2`).
- **Module options:** generic key/value editor for any module (values are JSON; unquoted text is a string).
- **Style:** optional managed block in `style.css`:
  - 15 predefined color schemes (Catppuccin x4, Gruvbox dark/light, Nord, Dracula, Tokyo Night,
    Solarized dark/light, Rose Pine, One Dark, Everforest, Kanagawa) or custom colors.
  - Font, size, opacity, module radius/padding, live preview over dark and light wallpapers.

## Readability guarantees

Whatever scheme or custom colors are chosen, the generated CSS is adjusted so that:

- bar text has at least 4.5:1 contrast with the bar background;
- opacity is raised if needed so text stays readable over any wallpaper;
- accent/warning/critical fills are at least 3:1 against the bar, with text on them at least 4.5:1.

Adjustments are listed in the Style tab. Your original color picks are kept in the file.

## Files touched

- `~/.config/waybar/config.jsonc` (or `config.json` / `config`): comments are not preserved on save;
  unknown keys are. A `.bak` copy is written first.
- `~/.config/waybar/style.css`: only the block between `/* waybar-settings:begin */` and
  `/* waybar-settings:end */`, and only if "Manage a settings block" is ticked. `.bak` copy first.
- If the config cannot be parsed, saving is disabled.

"Save & Reload" runs `pkill -USR2 waybar`.

## Build

Requires Qt6 Widgets and CMake.

    cmake -B build && cmake --build build
    ./build/waybar-settings
    sudo cmake --install build

## Packaging

`waybar-settings.spec` is for OpenMandriva (`BuildSystem: cmake`). Tag releases as `0.2`, `0.3`, ...
to match `Source0`. Verify with `abb build`.

## License

GPL-3.0-or-later (placeholder; add a LICENSE file and adjust as needed).
