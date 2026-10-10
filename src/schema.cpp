#include "schema.h"

#include <QHash>

namespace ws {
namespace {

// ---- terse builders for the option tables ---------------------------------
Opt b(const char *k, bool d, const char *h, const char *g = "General") {
  return {k, OptKind::Bool, d ? "true" : "false", h, QString(), g};
}
Opt i(const char *k, int d, const char *h, const char *g = "General") {
  return {k, OptKind::Int, QString::number(d), h, QString(), g};
}
Opt in(const char *k, const char *h, const char *g = "General") {
  return {k, OptKind::Int, QString(), h, QString(), g};
}
Opt d(const char *k, double dv, const char *h, const char *g = "General") {
  return {k, OptKind::Double, QString::number(dv), h, QString(), g};
}
Opt dn(const char *k, const char *h, const char *g = "General") {
  return {k, OptKind::Double, QString(), h, QString(), g};
}
Opt s(const char *k, const char *dv, const char *h, const char *g = "General") {
  return {k, OptKind::String, dv, h, QString(), g};
}
Opt sn(const char *k, const char *h, const char *g = "General") {
  return {k, OptKind::String, QString(), h, QString(), g};
}
Opt e(const char *k, const char *dv, const char *choices, const char *h, const char *g = "General") {
  return {k, OptKind::Choice, dv, h, choices, g};
}
Opt en(const char *k, const char *choices, const char *h, const char *g = "General") {
  return {k, OptKind::Choice, QString(), h, choices, g};
}
Opt l(const char *k, const char *h, const char *g = "General") {
  return {k, OptKind::List, QString(), h, QString(), g};
}
Opt j(const char *k, const char *h, const char *g = "General") {
  return {k, OptKind::Json, QString(), h, QString(), g};
}
Opt jd(const char *k, const char *dv, const char *h, const char *g = "General") {
  return {k, OptKind::Json, dv, h, QString(), g};
}
Opt c(const char *k, const char *h, const char *g = "Interactions") {
  return {k, OptKind::Command, QString(), h, QString(), g};
}
Opt p(const char *k, const char *h, const char *g = "General") {
  return {k, OptKind::Path, QString(), h, QString(), g};
}
Opt iv(const char *k, int dv, const char *h, const char *g = "General") {
  return {k, OptKind::Interval, QString::number(dv), h, QString(), g};
}
Opt ivn(const char *k, const char *h, const char *g = "General") {
  return {k, OptKind::Interval, QString(), h, QString(), g};
}
Opt st(const char *k, const char *h = "Each name becomes a CSS class you can style") {
  return {k, OptKind::States, QString(), h, QString(), QStringLiteral("Format")};
}

const char *kGen = "General";
const char *kApp = "Appearance";
const char *kFmt = "Format";
const char *kBeh = "Behaviour";
const char *kInt = "Interactions";
const char *kAdv = "Advanced";

const QVector<Opt> &commonModule() {
  static const QVector<Opt> v = {
      b("tooltip", true, "Show a tooltip when hovering", kGen),
      b("expand", false, "Absorb the remaining space of the section", kGen),
      b("disable-on-sleep", false, "Suspend updates while the screen is off", kGen),
      sn("cursor", "Cursor shape while hovering, e.g. pointer, grab, text", kGen),
      c("on-update", "Command run after every update", kGen),
      l("hosts", "Only enable this module on these hostnames", kAdv),
      j("actions", "Map of event name to built-in action, e.g. {\"on-click\":\"module:toggle\"}", kAdv),
  };
  return v;
}

const QVector<Opt> &commonLabel() {
  static const QVector<Opt> v = {
      sn("format", "Label text; {placeholders} are replaced at runtime", kFmt),
      sn("format-alt", "Alternative text toggled by format-alt-click", kFmt),
      e("format-alt-click", "click-left", "click-left|click-middle|click-right|click-backward|click-forward",
        "Button that switches to format-alt", kFmt),
      j("format-icons", "Thresholds (\"20 40 60\"), [{\"icon\":..,\"max\":..}] or an object keyed by alt/default", kFmt),
      sn("tooltip-format", "Tooltip text; placeholders as in format", kFmt),
      st("states", "Each name becomes a CSS class you can style"),
      ivn("interval", "Seconds, \"once\", or 0 for event driven updates", kBeh),
      in("min-length", "Minimum label width in characters", kApp),
      in("max-length", "Maximum label width in characters, ellipsised", kApp),
      i("rotate", 0, "Rotation: 0, 90, 180 or 270", kApp),
      dn("align", "0.0 = start of the bar, 1.0 = end", kApp),
      en("justify", "left|right|center", "Label alignment inside its module", kApp),
  };
  return v;
}

const QVector<Opt> &commonIcon() {
  static const QVector<Opt> v = {
      b("icon", false, "Show an icon next to the text", kApp),
      i("icon-size", 24, "Icon edge length in pixels", kApp),
      i("icon-spacing", 8, "Pixels between icon and text", kApp),
      b("swap-icon-label", false, "Put the icon after the text", kApp),
  };
  return v;
}

const QVector<Opt> &graphOpts() {
  static const QVector<Opt> v = {
      i("width", 100, "Graph width in pixels", kApp),
      i("datapoints", 20, "Number of samples kept", kBeh),
      e("graph_type", "line", "line|bar|gauge", "How the samples are drawn", kApp),
      iv("interval", 5, "Seconds between refreshes", kBeh),
  };
  return v;
}

const QVector<Opt> &sliderOpts() {
  static const QVector<Opt> v = {
      i("min", 0, "Minimum slider value", kApp),
      i("max", 100, "Maximum slider value", kApp),
      e("orientation", "horizontal", "horizontal|vertical", "Slider orientation", kApp),
  };
  return v;
}

const QVector<Opt> &eventOpts() {
  static const QVector<Opt> v = {
      c("on-click", "Left click", kInt),
      c("on-click-release", "Left button released", kInt),
      c("on-double-click", "Left double click", kInt),
      b("on-click-copy", false, "Left click copies the label text to the clipboard", kInt),
      c("on-click-middle", "Middle click", kInt),
      c("on-click-right", "Right click", kInt),
      c("on-double-click-right", "Right double click", kInt),
      c("on-click-backward", "Back / scroll away click", kInt),
      c("on-click-forward", "Forward click", kInt),
      c("on-scroll-up", "Wheel up", kInt),
      c("on-scroll-down", "Wheel down", kInt),
      c("on-scroll-left", "Wheel left", kInt),
      c("on-scroll-right", "Wheel right", kInt),
      b("reverse-scrolling", false, "Reverse scrolling for non-mouse devices", kAdv),
      b("reverse-mouse-scrolling", false, "Reverse scrolling for mice", kAdv),
      d("smooth-scrolling-threshold", 0.0, "Accumulated pixels needed for one step", kAdv),
  };
  return v;
}

// ---- module specific options ---------------------------------------------
const QVector<Opt> &optsBacklight() {
  static const QVector<Opt> v = {
      sn("device", "Backlight device, e.g. intel_backlight; empty autodetects", kGen),
      d("scroll-step", 1.0, "Brightness change per scroll step", kBeh),
      d("min-brightness", 0.0, "Smallest brightness the scroll steps can reach", kBeh),
  };
  return v;
}

const QVector<Opt> &optsBattery() {
  static const QVector<Opt> v = {
      s("bat", "", "Battery name, e.g. BAT0; empty autodetects", kGen),
      s("adapter", "", "Power adapter name, e.g. AC; empty autodetects", kGen),
      iv("interval", 60, "Seconds between refreshes", kBeh),
      in("full-at", "Capacity treated as full when plugged in", kBeh),
      b("full-at-plugged", false, "Report plugged as full above full-at", kBeh),
      b("design-capacity", false, "Report percentage of design capacity instead of the last full value", kBeh),
      b("weighted-average", true, "Use the charge-weighted average capacity", kBeh),
      b("smooth-power", false, "Use the battery's smooth power draw instead of an integral", kBeh),
      d("smooth-power-time-constant", 260.0, "Time constant used when smooth-power is on", kBeh),
      b("bat-compatibility", false, "Use the buggy firmware power_now interface", kAdv),
      s("format-time", "{H} h {M} min", "Remaining time format; keys {H} {M} {m}", kFmt),
      jd("events", "{}", "Status hooks, e.g. {\"on-charging\":\"notify-send Charging\"}", kAdv),
  };
  return v;
}

const QVector<Opt> &optsBluetooth() {
  static const QVector<Opt> v = {
      sn("controller", "Bluetooth controller address", kGen),
      sn("controller-alias", "Friendly controller name; takes precedence over controller", kGen),
      l("format-device-preference", "Preferred device order when several are paired", kFmt),
      sn("format-connected-battery", "Format for the battery level of a connected device", kFmt),
      sn("format-enumerate-connected", "Format for the list of connected devices", kFmt),
  };
  return v;
}

const QVector<Opt> &optsCava() {
  static const QVector<Opt> v = {
      p("cava_config", "Extra cava config file; JSON options here win", kGen),
      i("framerate", 30, "Frames per second", kBeh),
      i("autosens", 1, "1 off, 2 on, 3 strict", kBeh),
      i("sensitivity", 100, "Sensitivity in dB", kBeh),
      i("bars", 12, "Number of bars", kApp),
      i("lower_cutoff_freq", 50, "Lower frequency cut-off in Hz", kBeh),
      i("higher_cutoff_freq", 10000, "Higher frequency cut-off in Hz", kBeh),
      i("sleep_timer", 5, "Seconds of silence before sleeping; 0 never sleeps", kBeh),
      b("hide_on_silence", false, "Hide the module while cava sleeps", kBeh),
      sn("format_silent", "Text shown when silent (raw backend)", kFmt),
      e("method", "pulse", "pulse|pipewire|alsa|fifo|sndio|shmem", "Capture backend", kGen),
      s("source", "auto", "Capture source name", kGen),
      i("sample_rate", 44100, "Sample rate in Hz", kAdv),
      i("sample_bits", 16, "Bits per sample", kAdv),
      b("stereo", true, "Capture both channels", kAdv),
      b("reverse", false, "Flip the bars left to right", kApp),
      i("bar_delimiter", 0, "ASCII code inserted between bars; 0 for none", kApp),
      e("data_format", "ascii", "ascii|binary", "Bar value encoding", kAdv),
      sn("raw_target", "Destination of the raw backend", kAdv),
      b("monstercat", false, "Monstercat style bars", kApp),
      b("waves", false, "Wave style bars", kApp),
      d("noise_reduction", 0.77, "Noise reduction factor", kAdv),
      i("input_delay", 4, "Input delay in seconds", kAdv),
      i("bar_width", 0, "GLSL bar width in pixels", kApp),
      i("bar_spacing", 0, "GLSL spacing between bars", kApp),
      i("bar_height", 0, "Bar height in pixels", kApp),
      i("sdl_width", 0, "GLSL window width", kApp),
      i("sdl_height", 0, "GLSL window height", kApp),
      i("gradient", 0, "GLSL gradient flag", kApp),
      i("gradient_count", 0, "Number of gradient colours, max 8", kApp),
  };
  return v;
}

const QVector<Opt> &optsClock() {
  static const QVector<Opt> v = {
      iv("interval", 60, "Seconds between refreshes", kBeh),
      sn("timezone", "Timezone name, e.g. Europe/Berlin", kGen),
      l("timezones", "Timezones to cycle through; \"\" is local time", kGen),
      sn("timezone-tooltip-format", "Tooltip format listing the timezones", kFmt),
      s("locale", "", "Locale used for the date; empty means C", kGen),
      jd("calendar", "{}", "Calendar sub-object used when the tooltip shows {calendar}", kAdv),
  };
  return v;
}

const QVector<Opt> &optsDisk() {
  static const QVector<Opt> v = {
      l("paths", "Mount points to watch", kGen),
      s("header", "", "Text placed before the values", kFmt),
      s("separator", " ", "Separator between the listed paths", kFmt),
      en("unit", "B|kB|kiB|MB|MiB|GB|GiB|TB|TiB", "Byte unit; empty keeps raw bytes", kFmt),
  };
  return v;
}

const QVector<Opt> &optsGps() {
  static const QVector<Opt> v = {
      b("hide-disconnected", true, "Hide while no receiver is connected", kBeh),
      b("hide-no-fix", false, "Hide while there is no position fix", kBeh),
  };
  return v;
}

const QVector<Opt> &optsGamemode() {
  static const QVector<Opt> v = {
      s("glyph", "\xF0\x9F\x8A\xB4", "Character shown while no game runs", kFmt),
      b("use-icon", true, "Show an icon instead of the glyph", kApp),
      s("icon-name", "input-gaming-symbolic", "Icon name", kApp),
      i("icon-size", 20, "Icon edge length in pixels", kApp),
      i("icon-spacing", 4, "Pixels between icon and text", kApp),
      b("hide-not-running", true, "Hide the module when no game runs", kBeh),
  };
  return v;
}

const QVector<Opt> &optsGroup() {
  static const QVector<Opt> v = {
      l("modules", "Modules inside this group", kGen),
      e("orientation", "orthogonal", "inherit|orthogonal|vertical|horizontal", "Layout of the group", kApp),
      jd("drawer", "", "Drawer sub-object: {\"transition-duration\":500,\"click-to-reveal\":true,..}", kAdv),
  };
  return v;
}

const QVector<Opt> &optsIdleInhibitor() {
  static const QVector<Opt> v = {
      b("start-activated", false, "Start with the inhibitor already active", kBeh),
      d("timeout", 0.0, "Minutes until it turns itself off; 0 means never", kBeh),
      d("timeout-step", 10.0, "Minutes added or removed by scrolling", kBeh),
      b("dynamic-timeouts", false, "Show the remaining time", kBeh),
      b("wait-for-activity", false, "Wait for user activity before counting down", kBeh),
      in("signal", "RT signal number used to toggle the inhibitor", kAdv),
  };
  return v;
}

const QVector<Opt> &optsImage() {
  static const QVector<Opt> v = {
      p("path", "Image to show; ignored when exec is set", kGen),
      c("exec", "Script whose first output line is the image path", kGen),
      i("size", 16, "Image edge length in pixels", kApp),
      ivn("interval", "Seconds between updates, or \"once\"", kBeh),
      i("signal", 0, "RT signal used to refresh the image", kAdv),
      b("multiple", false, "Enable multi-image mode driven by entries or exec", kBeh),
      jd("entries", "[]", "Multi-image entries: [{\"path\":\"..\",\"marker\":\"..\",\"tooltip\":\"..\"}]", kAdv),
  };
  return v;
}

const QVector<Opt> &optsInhibitor() {
  static const QVector<Opt> v = {
      {"what", OptKind::List, QStringLiteral("[\"idle\"]"),
       QStringLiteral("Logind lock(s) to hold: idle, shutdown, sleep, handle-power-key, "
                      "handle-suspend-key, handle-hibernate-key, handle-lid-switch"),
       QString(), QStringLiteral("General")},
  };
  return v;
}

const QVector<Opt> &optsJack() {
  static const QVector<Opt> v = {
      b("realtime", true, "Request real-time scheduling", kBeh),
  };
  return v;
}

const QVector<Opt> &optsKeyboardState() {
  static const QVector<Opt> v = {
      jd("format", "{\"numlock\":\"{name} {icon}\",\"capslock\":\"{name} {icon}\",\"scrolllock\":\"{name} {icon}\"}",
         "Per-lock text, or a plain string", kFmt),
      jd("format-icons", "{\"locked\":\"locked\",\"unlocked\":\"unlocked\"}", "Icons for the lock states", kFmt),
      b("numlock", false, "Show the num lock indicator", kApp),
      b("capslock", false, "Show the caps lock indicator", kApp),
      b("scrolllock", false, "Show the scroll lock indicator", kApp),
      p("device-path", "Input device to watch; empty autodetects", kGen),
      jd("binding-keys", "[58,69,70]", "Key codes that act as num/caps/scroll lock", kAdv),
      b("disable-scroll", false, "Disable scrolling on the indicators", kBeh),
  };
  return v;
}

const QVector<Opt> &optsMemory() {
  static const QVector<Opt> v = {
      en("unit", "B|kB|kiB|MB|MiB|GB|GiB|TB|TiB", "Byte unit for {used} and {total}", kFmt),
  };
  return v;
}

const QVector<Opt> &optsMpd() {
  static const QVector<Opt> v = {
      sn("server", "MPD host name", kGen),
      i("port", 0, "MPD port; 0 uses the MPD default", kGen),
      s("password", "", "MPD password", kGen),
      iv("interval", 5, "Seconds between refreshes", kBeh),
      i("playing-interval", 1000, "Milliseconds between refreshes while playing", kBeh),
      i("timeout", 30, "Connection timeout in seconds", kBeh),
      s("unknown-tag", "N/A", "Text for missing tags", kFmt),
      s("format-stopped", "stopped", "Text while stopped", kFmt),
      s("format-paused", "paused", "Text while paused", kFmt),
      s("format-disconnected", "disconnected", "Text while disconnected", kFmt),
      s("tooltip-format-disconnected", "MPD (disconnected)", "Tooltip while disconnected", kFmt),
      j("state-icons", "Icons per state: {\"playing\":\"..\",\"paused\":\"..\"}", kFmt),
      j("consume-icons", "Icons for consume mode on/off", kFmt),
      j("random-icons", "Icons for random mode on/off", kFmt),
      j("repeat-icons", "Icons for repeat mode on/off", kFmt),
      j("single-icons", "Icons for single mode on/off", kFmt),
      in("artist-len", "Truncate the artist to this length", kApp),
      in("album-len", "Truncate the album to this length", kApp),
      in("album-artist-len", "Truncate the album artist to this length", kApp),
      in("title-len", "Truncate the title to this length", kApp),
      s("ellipsis", "", "Ellipsis appended when text is truncated", kApp),
  };
  return v;
}

const QVector<Opt> &optsMpris() {
  static const QVector<Opt> v = {
      s("player", "playerctld", "Player bus name; the default follows the active player", kGen),
      l("ignored-players", "Players to hide", kGen),
      iv("interval", 0, "Seconds between refreshes; 0 is event driven", kBeh),
      in("artist-len", "Truncate the artist to this length", kApp),
      in("album-len", "Truncate the album to this length", kApp),
      in("title-len", "Truncate the title to this length", kApp),
      in("dynamic-len", "Truncate the composed dynamic field to this length", kApp),
      l("dynamic-importance-order", "Field order in {dynamic}; default title,artist,album,position,length", kFmt),
      s("dynamic-separator", " - ", "Separator between the dynamic fields", kFmt),
      b("truncate-hours", true, "Drop the hours field when it is zero", kApp),
      b("enable-tooltip-len-limits", false, "Also apply the length limits to the tooltip", kApp),
      b("prefer-album-artist", false, "Use the album artist instead of the artist", kApp),
      s("ellipsis", "\xE2\x80\xA6", "Ellipsis appended when text is truncated", kApp),
      j("player-icons", "Icons per player", kFmt),
      j("status-icons", "Icons per status", kFmt),
  };
  return v;
}

const QVector<Opt> &optsNetwork() {
  static const QVector<Opt> v = {
      sn("interface", "Interface to watch; empty uses the default route", kGen),
      e("family", "ipv4", "ipv4|ipv6|ipv4_6", "Address family to report", kGen),
      b("rfkill", true, "Honour rfkill for wireless interfaces", kBeh),
  };
  return v;
}

const QVector<Opt> &optsPrivacy() {
  static const QVector<Opt> v = {
      i("icon-size", 20, "Icon edge length in pixels; 0 is automatic", kApp),
      i("icon-spacing", 4, "Pixels between icons", kApp),
      i("transition-duration", 250, "Icon fade duration in milliseconds", kApp),
      jd("modules", "[{\"type\":\"screenshare\"},{\"type\":\"audio-in\"},{\"type\":\"location\"}]",
         "Which privacy indicators to show", kGen),
      b("ignore-monitor", true, "Ignore monitor sources", kBeh),
      jd("ignore", "[]", "Streams to ignore: [{\"type\":\"audio-in\",\"name\":\"..\"}]", kAdv),
  };
  return v;
}

const QVector<Opt> &optsPulseaudio() {
  static const QVector<Opt> v = {
      e("target", "sink", "sink|source", "Speakers or microphone", kGen),
      d("scroll-step", 1.0, "Volume change per scroll step", kBeh),
      i("max-volume", 100, "Highest volume reachable by scrolling", kBeh),
      l("ignored-sinks", "Sinks to hide", kGen),
      j("sink-mapping", "Map a sink to a display name", kGen),
      s("format-source", "{volume}%", "Label while recording", kFmt),
      s("format-muted", "", "Label while muted", kFmt),
      s("format-source-muted", "", "Label while the source is muted", kFmt),
      s("format-bluetooth", "", "Label for Bluetooth devices", kFmt),
      s("format-bluetooth-muted", "", "Label for muted Bluetooth devices", kFmt),
  };
  return v;
}

const QVector<Opt> &optsPulseaudioSlider() {
  static const QVector<Opt> v = {
      e("target", "sink", "sink|source", "Speakers or microphone", kGen),
      b("zero-on-mute", true, "Drop the slider to zero when muted", kBeh),
      b("unmute-on-volume-change", true, "Unmute when the volume is changed", kBeh),
      l("ignored-sinks", "Sinks to hide", kGen),
      j("sink-mapping", "Map a sink to a display name", kGen),
  };
  return v;
}

const QVector<Opt> &optsWireplumber() {
  static const QVector<Opt> v = {
      e("node-type", "Audio/Sink", "Audio/Sink|Audio/Source", "Which node to follow", kGen),
      b("only-physical", false, "Only follow physical devices", kBeh),
      d("scroll-step", 1.0, "Volume change per scroll step", kBeh),
      e("scroll-scale", "cubic_percent", "cubic|db|cubic_percent", "Volume scale while scrolling", kBeh),
      d("max-volume", 100.0, "Highest volume reachable by scrolling", kBeh),
      s("format-muted", "", "Label while muted", kFmt),
      s("format-source", "{volume}%", "Label while recording", kFmt),
      s("format-source-muted", "", "Label while the source is muted", kFmt),
      s("format-bluetooth", "", "Label for Bluetooth devices", kFmt),
      s("format-bluetooth-muted", "", "Label for muted Bluetooth devices", kFmt),
  };
  return v;
}

const QVector<Opt> &optsPrivacyExtras() { return optsPrivacy(); }

const QVector<Opt> &optsSndio() {
  static const QVector<Opt> v = {
      i("scroll-step", 5, "Volume change per scroll step", kBeh),
  };
  return v;
}

const QVector<Opt> &optsSystemdFailed() {
  static const QVector<Opt> v = {
      b("user", true, "Watch the user manager", kGen),
      b("system", true, "Watch the system manager", kGen),
      b("hide-on-ok", true, "Hide the module when nothing failed", kBeh),
      s("tooltip-unit-format", "{name}: {description}", "Tooltip line for every failed unit", kFmt),
  };
  return v;
}

const QVector<Opt> &optsTemperature() {
  static const QVector<Opt> v = {
      i("thermal-zone", 0, "Thermal zone index", kGen),
      p("hwmon-path", "Path of the hwmon directory", kGen),
      p("hwmon-path-abs", "Absolute path of the hwmon directory", kGen),
      sn("hwmon-name", "Sensor name to look for", kGen),
      sn("input-filename", "Sensor file name, e.g. temp1_input", kGen),
      sn("hwmon-by-name", "Legacy substring match for the sensor name", kAdv),
      in("warning-threshold", "Temperature at which the warning class appears", kBeh),
      in("critical-threshold", "Temperature at which the critical class appears", kBeh),
      s("format-warning", "", "Label while warning", kFmt),
      s("format-critical", "", "Label while critical", kFmt),
  };
  return v;
}

const QVector<Opt> &optsTray() {
  static const QVector<Opt> v = {
      i("icon-size", 16, "Icon edge length in pixels", kApp),
      i("spacing", 0, "Pixels between tray icons", kApp),
      b("reverse-direction", false, "Pack new icons to the left", kApp),
      b("show-passive-items", false, "Also show passive tray items", kBeh),
      j("icons", "Map an app-id to an icon name, or false to hide it", kAdv),
      j("orders", "Map an item name to an integer; higher is further right", kAdv),
      l("ignore-list", "Substrings; matching items are never shown", kAdv),
  };
  return v;
}

const QVector<Opt> &optsUpower() {
  static const QVector<Opt> v = {
      p("native-path", "UPower device native path", kGen),
      sn("model", "UPower device model name", kGen),
      i("icon-size", 20, "Icon edge length in pixels", kApp),
      b("show-icon", true, "Show the battery icon", kApp),
      b("hide-if-empty", true, "Hide the module when it has no battery", kBeh),
      i("tooltip-spacing", 4, "Line spacing inside the tooltip", kApp),
      i("tooltip-padding", 4, "Padding inside the tooltip", kApp),
  };
  return v;
}

const QVector<Opt> &optsUser() {
  static const QVector<Opt> v = {
      p("avatar", "Path of the avatar image", kGen),
      i("height", 20, "Avatar height in pixels", kApp),
      i("width", 20, "Avatar width in pixels", kApp),
      b("open-on-click", false, "Open a file manager on click", kBeh),
      p("open-path", "Directory opened on click; empty is your home", kBeh),
  };
  return v;
}

const QVector<Opt> &optsTaskbar() {
  static const QVector<Opt> v = {
      l("icon-theme", "Icon themes to look through", kGen),
      i("icon-size", 16, "Icon edge length in pixels", kApp),
      b("markup", false, "Interpret pango markup in window titles", kApp),
      sn("tooltip-format", "Tooltip format", kFmt),
      b("all-outputs", false, "Show windows from every output", kBeh),
      b("bar-css-states", false, "Mirror window states onto the bar itself", kBeh),
      b("active-first", false, "Keep the active window at the front", kBeh),
      b("active-only", false, "Only show the active window", kBeh),
      b("sort-by-app-id", false, "Group windows by application", kBeh),
      b("homogeneous", false, "Use the same button size for every window", kApp),
      b("truncate", false, "Truncate long titles", kApp),
      en("justify", "left|right|center", "Task button alignment", kApp),
      l("ignore-list", "Substrings; matching windows are hidden", kAdv),
      l("squash-list", "Substrings; matching windows are merged; \"*\" merges all", kAdv),
      j("app_ids-mapping", "Map an app id to a display name", kAdv),
      j("rewrite", "Map a title substring to its replacement", kAdv),
  };
  return v;
}

const QVector<Opt> &optsWwan() {
  static const QVector<Opt> v = {
      sn("imei", "Modem IMEI to follow", kGen),
      p("path", "Physical device path of the modem", kGen),
      b("hide-disconnected", true, "Hide while disconnected", kBeh),
  };
  return v;
}

const QVector<Opt> &optsSwayWorkspaces() {
  static const QVector<Opt> v = {
      s("format-for-negative-index", "{value}", "Format used for automatically numbered workspaces", kFmt),
      s("format-window-separator", " ", "Separator between window indicators", kFmt),
      b("all-outputs", false, "Show workspaces of every output", kBeh),
      b("current-only", false, "Only show workspaces of the current output", kBeh),
      l("ignore-workspaces", "Workspace names to hide (regular expressions)", kGen),
      j("persistent-workspaces", "Map a workspace name to the outputs that always show it", kGen),
      b("disable-scroll", false, "Disable scrolling between workspaces", kBeh),
      b("reverse-scroll", false, "Reverse the scroll direction", kBeh),
      b("warp-on-scroll", true, "Move the pointer when scrolling", kBeh),
      b("disable-scroll-wraparound", false, "Stop at the first and last workspace", kBeh),
      b("enable-bar-scroll", false, "Scroll anywhere on the bar", kBeh),
      b("disable-click", false, "Disable switching workspaces by clicking", kBeh),
      b("disable-auto-back-and-forth", false, "Do not return to the current workspace", kBeh),
      b("no-switch-output", false, "Do not switch output when clicking a workspace", kBeh),
      b("disable-markup", false, "Treat titles as plain text", kBeh),
      b("alphabetical_sort", false, "Sort workspaces by name", kBeh),
      l("custom-sort", "Explicit workspace order", kBeh),
      j("output-classes", "Map an output name to a CSS class", kAdv),
      j("window-rewrite", "Map a window title substring to its replacement", kAdv),
      s("window-rewrite-default", "?", "Text for titles that match nothing", kAdv),
  };
  return v;
}

const QVector<Opt> &optsSwayWindow() {
  static const QVector<Opt> v = {
      b("all-outputs", false, "Show windows of every output", kBeh),
      b("offscreen-css", false, "Style the module while the window is offscreen", kBeh),
      sn("offscreen-css-text", "Text shown while the window is offscreen", kFmt),
      b("show-focused-workspace-name", false, "Prefix the title with the workspace name", kFmt),
      b("show-hidden-marks", false, "Show the hidden window markers", kApp),
      j("rewrite", "Map a window title substring to its replacement", kAdv),
  };
  return v;
}

const QVector<Opt> &optsHyprWorkspaces() {
  static const QVector<Opt> v = {
      s("format-window-separator", " ", "Separator between window indicators", kFmt),
      j("tooltips", "Map a workspace name to its tooltip", kFmt),
      j("window-rewrite", "Map a window title substring to its replacement", kAdv),
      s("window-rewrite-default", "?", "Text for titles that match nothing", kAdv),
      i("window-rewrite-group-threshold", 0, "Group windows after this many", kAdv),
      s("window-rewrite-group-format", "{icon}\xC3\x97{count}", "Format used once windows are grouped", kAdv),
      j("workspace-taskbar", "Taskbar sub-object shown inside a workspace button", kAdv),
      i("max-windows", 0, "Window indicators per workspace; 0 is unlimited", kApp),
      b("show-special", false, "Show special workspaces", kBeh),
      b("special-visible-only", false, "Only show special workspaces when they are visible", kBeh),
      b("persistent-only", false, "Only show persistent workspaces", kBeh),
      j("persistent-workspaces", "Map a workspace name to the outputs that always show it", kGen),
      b("all-outputs", false, "Show workspaces of every output", kBeh),
      b("active-only", false, "Only show workspaces of the active output", kBeh),
      b("hide-active", false, "Hide the active workspace", kBeh),
      b("move-to-monitor", false, "Move the pointer when a workspace is clicked", kBeh),
      b("unique-icons", false, "Pick icons so adjacent workspaces differ", kApp),
      b("enable-bar-scroll", false, "Scroll anywhere on the bar", kBeh),
      l("ignore-workspaces", "Workspace names to hide (regular expressions)", kGen),
      e("sort-by", "DEFAULT", "ID|NAME|NUMBER|SPECIAL-CENTERED|DEFAULT", "Workspace ordering", kBeh),
  };
  return v;
}

const QVector<Opt> &optsHyprWindow() {
  static const QVector<Opt> v = {
      j("rewrite", "Map a window title substring to its replacement", kAdv),
      b("separate-outputs", false, "Show windows of every output separately", kBeh),
      sn("fallback", "Text shown when the title is empty", kFmt),
  };
  return v;
}

const QVector<Opt> &optsWindowCommon() {
  static const QVector<Opt> v = {
      j("rewrite", "Map a window title substring to its replacement", kAdv),
      b("separate-outputs", false, "Show windows of every output separately", kBeh),
      b("show-empty", false, "Keep the module visible when there is no window", kBeh),
      sn("fallback", "Text shown when the title is empty", kFmt),
  };
  return v;
}

const QVector<Opt> &optsLanguage() {
  static const QVector<Opt> v = {
      sn("keyboard-name", "Keyboard to follow; empty follows the focused one", kGen),
      b("hide-single-layout", false, "Hide while only one layout is available", kBeh),
  };
  return v;
}

const QVector<Opt> &optsSubmap() {
  static const QVector<Opt> v = {
      j("icons", "Map a submap name to its icon", kFmt),
      b("always-on", false, "Keep the module visible in the default submap", kBeh),
      s("default-submap", "", "Name of the default submap", kGen),
  };
  return v;
}

const QVector<Opt> &optsNiriWorkspaces() {
  static const QVector<Opt> v = {
      s("format-window-separator", " ", "Separator between window indicators", kFmt),
      j("window-rewrite", "Map a window title substring to its replacement", kAdv),
      s("window-rewrite-default", "?", "Text for titles that match nothing", kAdv),
      j("workspace-taskbar", "Taskbar sub-object shown inside a workspace button", kAdv),
      b("all-outputs", false, "Show workspaces of every output", kBeh),
      e("display-condition", "show-all", "show-all|only-populated|keep-named", "Which workspaces to list", kBeh),
      b("sort-by-name", false, "Sort workspaces by name", kBeh),
      b("sort-by-coordinates", false, "Sort workspaces by position", kBeh),
      b("sort-by-id", false, "Sort workspaces by id", kBeh),
      b("current-only", false, "Only show workspaces of the current output", kBeh),
      b("hide-empty", false, "Hide workspaces without windows", kBeh),
      b("disable-click", false, "Disable switching workspaces by clicking", kBeh),
      b("disable-markup", false, "Treat titles as plain text", kBeh),
      b("enable-bar-scroll", false, "Scroll anywhere on the bar", kBeh),
      l("ignore-workspaces", "Workspace names to hide (regular expressions)", kGen),
  };
  return v;
}

const QVector<Opt> &optsTags() {
  static const QVector<Opt> v = {
      i("num-tags", 9, "Number of tags, capped at 32", kGen),
      l("tag-labels", "Labels shown instead of 1,2,3", kFmt),
      j("set-tags", "Bitmask assigned when a tag is clicked", kAdv),
      j("toggle-tags", "Bitmask toggled when a tag is clicked", kAdv),
      b("hide-vacant", false, "Hide tags without windows", kBeh),
      b("disable-click", false, "Disable switching tags by clicking", kBeh),
  };
  return v;
}

const QVector<Opt> &optsRiverWindow() {
  static const QVector<Opt> v = {
      b("tooltip", true, "Show a tooltip with the window title", kGen),
  };
  return v;
}

const QVector<Opt> &optsExtWorkspaces() {
  static const QVector<Opt> v = {
      b("sort-by-name", true, "Sort workspaces by name", kBeh),
      b("sort-by-coordinates", false, "Sort workspaces by position", kBeh),
      b("sort-by-id", false, "Sort workspaces by id", kBeh),
      b("all-outputs", false, "Show workspaces of every output", kBeh),
      b("active-only", false, "Only show workspaces of the current output", kBeh),
      b("ignore-hidden", true, "Hide workspaces hidden by the compositor", kBeh),
      b("disable-click", false, "Disable switching workspaces by clicking", kBeh),
      b("disable-markup", false, "Treat titles as plain text", kBeh),
  };
  return v;
}

const QVector<Opt> &optsWayfireWorkspaces() {
  static const QVector<Opt> v = {
      b("disable-click", false, "Disable switching workspaces by clicking", kBeh),
      b("disable-scroll", false, "Disable scrolling between workspaces", kBeh),
      b("enable-bar-scroll", false, "Scroll anywhere on the bar", kBeh),
      b("disable-markup", false, "Treat titles as plain text", kBeh),
      b("current-only", false, "Only show workspaces of the current output", kBeh),
  };
  return v;
}

const QVector<Opt> &optsMangoWorkspaces() {
  static const QVector<Opt> v = {
      s("overview-label", "OVERVIEW", "Label of the overview workspace button", kFmt),
      b("disable-markup", false, "Treat titles as plain text", kBeh),
      b("current-only", false, "Only show workspaces of the current output", kBeh),
      b("hide-empty", false, "Hide workspaces without windows", kBeh),
  };
  return v;
}

const QVector<Opt> &optsCustom() {
  static const QVector<Opt> v = {
      c("exec", "Script whose output becomes the label", kGen),
      c("exec-if", "Run exec only when this command exits with 0", kGen),
      b("exec-on-event", true, "Re-run the script after an event command", kBeh),
      e("return-type", "raw", "json|raw", "How the script output is interpreted", kGen),
      ivn("interval", "Seconds between runs, or \"once\"", kBeh),
      in("restart-interval", "Restart the script after this many seconds; 0 disables", kBeh),
      in("signal", "RT signal number used to refresh the label", kAdv),
      b("hide-empty-text", false, "Hide the module when the script prints no text", kBeh),
      b("escape", false, "Escape the script output as plain text", kBeh),
      p("image-path", "Image shown next to the text", kApp),
      sn("image-name", "Named image theme entry to use", kApp),
      i("icon-size", 24, "Icon edge length in pixels", kApp),
  };
  return v;
}

}  // namespace

namespace {

QVector<ModuleDef> buildModules() {
  QVector<ModuleDef> v;

  auto add = [&](const char *id, const char *label, const char *cat, const char *summary,
                 const QStringList &states, QVector<Opt> opts = {}, int flags = 0) {
    ModuleDef m;
    m.id = id;
    m.label = label;
    m.category = cat;
    m.summary = summary;
    m.states = states;
    m.opts = opts;
    m.labelBased = (flags & 1) != 0;
    m.iconBased = (flags & 2) != 0;
    m.graph = (flags & 4) != 0;
    m.slider = (flags & 8) != 0;
    v.append(m);
  };

  // System ---------------------------------------------------------------
  add("clock", "Clock", "System", "Time and date", {}, optsClock(), 1);
  add("cpu", "CPU usage", "System", "Total CPU load", {});
  add("cpu_usage", "CPU usage", "System", "Per-core CPU load", {});
  add("cpu_frequency", "CPU frequency", "System", "Current CPU frequency in GHz", {});
  add("cpu_graph", "CPU graph", "System", "CPU load history", {"cpu-moderate", "cpu-high", "cpu-intensive"},
      graphOpts(), 4);
  add("memory", "Memory", "System", "RAM and swap usage", {}, optsMemory());
  add("load", "Load average", "System", "1, 5 and 15 minute load", {});
  add("disk", "Disk", "System", "Free space per mount point", {}, optsDisk());
  add("temperature", "Temperature", "System", "Thermal sensor reading",
      {"warning", "critical"}, optsTemperature());
  add("user", "User", "System", "User name, avatar and session info", {}, optsUser(), 3);
  add("image", "Image", "System", "Static or script driven image", {"empty"}, optsImage());
  add("privacy", "Privacy", "System", "Screenshare, microphone and location indicators",
      {"screenshare", "audio-in", "audio-out", "location"}, optsPrivacyExtras());
  add("systemd-failed-units", "Failed units", "System", "Failed systemd units", {"ok", "degraded"},
      optsSystemdFailed());
  add("gamemode", "Game mode", "System", "Feral GameMode indicator", {"running"}, optsGamemode(), 2);
  add("keyboard-state", "Keyboard state", "System", "Num, caps and scroll lock",
      {"numlock", "capslock", "scrolllock", "locked"}, optsKeyboardState());
  add("idle_inhibitor", "Idle inhibitor", "System", "Inhibit idling and show the countdown",
      {"activated", "deactivated"}, optsIdleInhibitor());
  add("inhibitor", "Logind inhibitor", "System", "Hold a logind lock", {"activated", "deactivated"},
      optsInhibitor());
  add("gps", "GPS", "System", "Fix status of a gpsd receiver",
      {"fix-none", "fix-2d", "fix-3d", "disconnected", "disabled"}, optsGps());
  add("wwan", "Modem", "System", "ModemManager state",
      {"failed", "locked", "disabled", "enabled", "searching", "registered", "connected"}, optsWwan());

  // Audio, media and display ---------------------------------------------
  add("backlight", "Backlight", "Display", "Screen brightness", {}, optsBacklight());
  add("backlight/slider", "Backlight slider", "Display", "Screen brightness as a slider",
      {}, optsBacklight(), 8);
  add("pulseaudio", "Volume", "Audio", "PulseAudio or PipeWire sink volume",
      {"bluetooth", "muted", "sink-muted", "source-muted"}, optsPulseaudio(), 2);
  add("pulseaudio/slider", "Volume slider", "Audio", "PulseAudio sink volume as a slider",
      {"bluetooth", "muted", "sink-muted", "source-muted"}, optsPulseaudioSlider(), 8);
  add("wireplumber", "Volume", "Audio", "PipeWire volume through WirePlumber",
      {"bluetooth", "muted", "sink-muted", "source-muted"}, optsWireplumber(), 2);
  add("sndio", "Volume", "Audio", "sndio mixer volume", {"muted"}, optsSndio());
  add("jack", "JACK", "Audio", "JACK client load and xruns", {"connected", "disconnected", "xrun"},
      optsJack());
  add("mpd", "Music", "Audio", "MPD now playing", {"disconnected", "stopped", "playing", "paused"},
      optsMpd());
  add("mpris", "Media player", "Audio", "MPRIS media players", {"playing", "paused", "stopped"},
      optsMpris());
  add("cava", "Audio visualiser", "Audio", "Cava spectrum analyser", {"silent", "updated"},
      optsCava());

  // Network ---------------------------------------------------------------
  add("network", "Network", "Network", "Interface, address and link speed",
      {"disabled", "disconnected", "linked", "ethernet", "wifi"}, optsNetwork());
  add("bluetooth", "Bluetooth", "Network", "Controller and paired devices",
      {"discoverable", "discovering", "pairable", "disabled", "off", "on", "connected", "no-controller"},
      optsBluetooth());

  // Power -----------------------------------------------------------------
  add("battery", "Battery", "Power", "Charge, state and remaining time",
      {"charging", "discharging", "full", "plugged", "empty", "unknown"}, optsBattery(), 2);
  add("upower", "Battery", "Power", "UPower managed battery",
      {"charging", "discharging", "full", "empty", "unknown-status"}, optsUpower(), 2);
  add("power-profiles-daemon", "Power profile", "Power", "Current power profile",
      {"performance", "balanced", "power-saver"}, {});

  // Tray and user defined -------------------------------------------------
  add("tray", "Tray", "Tray", "StatusNotifier tray", {"passive", "active", "needs-attention"},
      optsTray(), 2);

  // Compositors -----------------------------------------------------------
  add("sway/mode", "Mode", "Sway", "Current sway mode", {"default", "floating"});
  add("sway/workspaces", "Workspaces", "Sway", "Sway workspaces",
      {"visible", "focused", "urgent", "persistent", "empty", "current_output"}, optsSwayWorkspaces());
  add("sway/window", "Focused window", "Sway", "Title of the focused window",
      {"empty", "solo", "floating", "tabbed", "stacked", "tiled"}, optsSwayWindow(), 2);
  add("sway/language", "Language", "Sway", "Keyboard layout", {}, optsLanguage());
  add("sway/scratchpad", "Scratchpad", "Sway", "Hidden windows",
      {"empty"}, {b("show-empty", false, "Show while the scratchpad is empty", kBeh)});

  add("hyprland/workspaces", "Workspaces", "Hyprland", "Hyprland workspaces",
      {"active", "special", "empty", "persistent", "urgent", "visible", "hosting-monitor"},
      optsHyprWorkspaces());
  add("hyprland/window", "Focused window", "Hyprland", "Title of the focused window",
      {"empty", "solo", "floating", "swallowing", "fullscreen"}, optsHyprWindow(), 2);
  add("hyprland/windowcount", "Window count", "Hyprland", "Number of windows",
      {"empty", "fullscreen"}, optsWindowCommon(), 2);
  add("hyprland/language", "Language", "Hyprland", "Keyboard layout", {}, optsLanguage());
  add("hyprland/submap", "Submap", "Hyprland", "Current submap", {}, optsSubmap());

  add("niri/workspaces", "Workspaces", "Niri", "Niri workspaces",
      {"focused", "active", "urgent", "empty", "current_output"}, optsNiriWorkspaces());
  add("niri/window", "Focused window", "Niri", "Title of the focused window", {"empty", "solo"},
      optsWindowCommon(), 2);
  add("niri/language", "Language", "Niri", "Keyboard layout", {}, optsLanguage());

  add("river/tags", "Tags", "River", "River tags", {"occupied", "focused", "urgent", "output"},
      optsTags());
  add("river/window", "Focused window", "River", "Title of the focused window", {"focused"},
      optsRiverWindow(), 2);
  add("river/layout", "Layout", "River", "Current layout", {"focused"});
  add("river/mode", "Mode", "River", "Current mode", {});

  add("dwl/tags", "Tags", "dwl", "dwl tags", {"occupied", "empty", "focused", "urgent", "output"},
      optsTags());
  add("dwl/window", "Focused window", "dwl", "Title of the focused window", {"active"},
      optsWindowCommon(), 2);

  add("mango/workspaces", "Workspaces", "Mango", "Mango workspaces",
      {"active", "urgent", "empty", "current_output", "overview"}, optsMangoWorkspaces());
  add("mango/window", "Focused window", "Mango", "Title of the focused window", {"empty", "solo"},
      optsWindowCommon(), 2);
  add("mango/language", "Language", "Mango", "Keyboard layout", {}, optsLanguage());
  add("mango/keymode", "Keymode", "Mango", "Current keymode", {});
  add("mango/layout", "Layout", "Mango", "Current layout symbol", {});

  add("wayfire/workspaces", "Workspaces", "Wayfire", "Wayfire workspaces",
      {"focused", "empty", "current_output"}, optsWayfireWorkspaces());
  add("wayfire/window", "Focused window", "Wayfire", "Title of the focused window", {"empty", "solo"},
      optsWindowCommon(), 2);

  add("ext/workspaces", "Workspaces", "wlroots", "Workspaces from the ext-workspace protocol",
      {"active", "urgent", "hidden"}, optsExtWorkspaces());
  add("wlr/taskbar", "Taskbar", "wlroots", "wlroots taskbar",
      {"maximized", "minimized", "active", "fullscreen", "empty"}, optsTaskbar());

  // Templates -------------------------------------------------------------
  {
    ModuleDef m;
    m.id = "custom/*";
    m.label = "Script";
    m.category = "Custom";
    m.summary = "Text or icon produced by a shell script";
    m.states = QStringList() << "flat" << "text-button";
    m.opts = optsCustom();
    m.labelBased = true;
    m.iconBased = true;
    m.template_ = true;
    m.suggestedName = QStringLiteral("custom/indicator");
    v.append(m);
  }
  {
    ModuleDef m;
    m.id = "custom-graph/*";
    m.label = "Script graph";
    m.category = "Custom";
    m.summary = "Script output drawn as a graph";
    m.states = QStringList() << "flat";
    m.opts = optsCustom();
    m.graph = true;
    m.template_ = true;
    m.suggestedName = QStringLiteral("custom-graph/history");
    v.append(m);
  }
  {
    ModuleDef m;
    m.id = "group/*";
    m.label = "Group";
    m.category = "Custom";
    m.summary = "Bundle several modules into one container";
    m.states = QStringList() << "empty" << "expanded" << "drawer";
    m.opts = optsGroup();
    m.labelBased = false;
    m.template_ = true;
    m.suggestedName = QStringLiteral("group/hardware");
    v.append(m);
  }
  {
    ModuleDef m;
    m.id = "cffi/*";
    m.label = "CFFI module";
    m.category = "Custom";
    m.summary = "Module loaded from a shared library";
    m.states = QStringList() << "module";
    m.labelBased = false;
    m.template_ = true;
    m.suggestedName = QStringLiteral("cffi/module");
    m.opts = {p("module_path", "Path of the shared library implementing this module", kGen)};
    v.append(m);
  }

  for (ModuleDef &m : v) m.cssId = cssIdFor(m.id);
  return v;
}

}  // namespace

// ---- public API -----------------------------------------------------------

const QVector<ModuleDef> &modules() {
  static const QVector<ModuleDef> v = buildModules();
  return v;
}

const ModuleDef *findModule(const QString &id) {
  for (const ModuleDef &m : modules())
    if (m.id == id) return &m;
  return nullptr;
}

QString moduleType(const QString &ref) {
  QString s = ref.trimmed();
  const int h = s.indexOf('#');
  if (h >= 0) s = s.left(h);
  if (s.startsWith(QLatin1String("custom-graph/"))) return QStringLiteral("custom-graph/*");
  if (s.startsWith(QLatin1String("custom/"))) return QStringLiteral("custom/*");
  if (s.startsWith(QLatin1String("cffi/"))) return QStringLiteral("cffi/*");
  if (s.startsWith(QLatin1String("group/"))) return QStringLiteral("group/*");
  return s;
}

QString cssIdFor(const QString &ref) {
  QString s = ref;
  const int h = s.indexOf('#');
  if (h >= 0) s = s.left(h);
  if (s.startsWith(QLatin1String("custom-graph/"))) return QStringLiteral("custom-graph-") + s.mid(13);
  if (s.startsWith(QLatin1String("custom/"))) return QStringLiteral("custom-") + s.mid(7);
  if (s.startsWith(QLatin1String("cffi/")) || s.startsWith(QLatin1String("group/"))) return s.mid(s.indexOf('/') + 1);
  const int sl = s.lastIndexOf('/');
  return sl >= 0 ? s.mid(sl + 1) : s;
}

QString cssSelector(const QString &ref) {
  QString s = ref;
  QString alias;
  const int h = s.indexOf('#');
  if (h >= 0) {
    alias = s.mid(h + 1);
    s = s.left(h);
  }
  QString out = QLatin1Char('#') + cssIdFor(s);
  if (!alias.isEmpty()) out += QLatin1Char('.') + alias;
  return out;
}

QString displayName(const QString &ref) {
  const ModuleDef *m = findModule(moduleType(ref));
  QString base = m ? m->label : ref;
  const int sl = ref.indexOf('/');
  if (sl > 0) {
    const QString ws = ref.left(sl);
    const ModuleDef *m2 = findModule(ws);
    const QString compositor = m2 ? m2->category : ws;
    return QStringLiteral("%1 (%2)").arg(base, compositor);
  }
  return base;
}

QString sampleText(const QString &ref) {
  const QString id = cssIdFor(ref);
  if (id.startsWith(QLatin1String("custom-graph-")) || id.startsWith(QLatin1String("cpu_graph")) ||
      id.startsWith(QLatin1String("cava")))
    return QStringLiteral("graph");
  static const QHash<QString, QString> t = {
      {"clock", QStringLiteral("12:34")},
      {"cpu", QStringLiteral("8%")},
      {"cpu_usage", QStringLiteral("12%")},
      {"cpu_frequency", QStringLiteral("3.4GHz")},
      {"memory", QStringLiteral("3.2 GiB")},
      {"load", QStringLiteral("0.52")},
      {"disk", QStringLiteral("42%")},
      {"temperature", QStringLiteral("48\xC2\xB0C")},
      {"user", QStringLiteral("ZIKY")},
      {"battery", QStringLiteral("87%")},
      {"upower", QStringLiteral("87%")},
      {"network", QStringLiteral("wlan0")},
      {"bluetooth", QStringLiteral("connected")},
      {"backlight", QStringLiteral("40%")},
      {"backlight-slider", QStringLiteral("slider")},
      {"pulseaudio", QStringLiteral("40%")},
      {"pulseaudio-slider", QStringLiteral("slider")},
      {"wireplumber", QStringLiteral("40%")},
      {"sndio", QStringLiteral("40%")},
      {"jack", QStringLiteral("12%")},
      {"mpd", QStringLiteral("Artist - Title")},
      {"mpris", QStringLiteral("Artist - Title")},
      {"tray", QStringLiteral("tray")},
      {"image", QStringLiteral("image")},
      {"privacy", QStringLiteral("privacy")},
      {"systemd-failed-units", QStringLiteral("0 failed")},
      {"gamemode", QStringLiteral("gaming")},
      {"keyboard-state", QStringLiteral("NUM")},
      {"idle_inhibitor", QStringLiteral("active")},
      {"inhibitor", QStringLiteral("idle")},
      {"gps", QStringLiteral("3D Fix")},
      {"wwan", QStringLiteral("Connected")},
      {"power-profiles-daemon", QStringLiteral("balanced")},
      {"workspaces", QStringLiteral("1 2 3")},
      {"tags", QStringLiteral("1 2 3")},
      {"window", QStringLiteral("Focused window")},
      {"taskbar", QStringLiteral("windows")},
      {"language", QStringLiteral("us")},
      {"mode", QStringLiteral("TILE")},
      {"keymode", QStringLiteral("LOGIN")},
      {"submap", QStringLiteral("default")},
      {"layout", QStringLiteral("main")},
      {"scratchpad", QStringLiteral("2")},
      {"windowcount", QStringLiteral("3")},
  };
  return t.value(id, id);
}

bool isTemplateRef(const QString &ref) {
  return moduleType(ref).endsWith(QLatin1Char('*'));
}

QVector<Opt> optionsFor(const ModuleDef &m) {
  QVector<Opt> out;
  auto append = [&out](const QVector<Opt> &v) {
    for (const Opt &o : v) {
      bool replaced = false;
      for (Opt &e : out)
        if (e.key == o.key) {
          e = o;
          replaced = true;
          break;
        }
      if (!replaced) out.append(o);
    }
  };
  append(commonModule());
  if (m.labelBased) append(commonLabel());
  if (m.iconBased) append(commonIcon());
  if (m.graph) append(graphOpts());
  if (m.slider) append(sliderOpts());
  append(m.opts);
  append(eventOpts());
  return out;
}

}  // namespace ws
