# kai — clock/calendar popup, hover animation, launcher placement

Date: 2026-11-14 · Status: draft for approval · Owner: Rune Balstad

## Intent

Three UX upgrades to the kai dock, requested by Rune after daily use:

1. The clock applet reads HH:MM:SS and opens a month calendar popup on click.
2. A new hover animation setting: an icon reacts when the pointer enters it
   (must-have: bounce; more choices wanted).
3. The apps launcher applet becomes the first item of the dock, carrying the
   Omarchy logo as its icon.

## Goals

- Digital clock face shows hours, minutes and seconds when `clockSeconds`
  is on (Rune's config has it on). Analog mode is untouched.
- Clicking the clock opens an in-dock month calendar popup: current month,
  today accented, previous/next month navigation, localized via `Qt.locale()`.
- `hoverAnimation` setting with choices `none / bounce / pulse / flip /
  wobble / blink`, default `bounce`. Plays once when the pointer enters any
  dock icon (apps and applets, not separators). Plays nothing while a launch
  or attention loop is already running on that icon.
- Dock model places the `apps` applet always as item 0, before all launchers;
  the remaining applets stay after the separator on the right side. The
  launcher icon becomes `/usr/share/omarchy/logo.svg`, falling back to the
  existing grid glyph when the file is missing.
- `en.json` / `no.json` catalogs stay valid; any new user-facing string is
  translated in `no.json`.

## Non-goals

- No event or task integration inside the calendar popup (libecal sync is
  heavy; revisit as a separate feature).
- No new animation types — hover reuses the five existing DockItem animations.
- No workspaces applet (dropped by decision of the owner).
- No change to applet right-click menus or desklet windows beyond what the
  clock change requires.

## Anti-goals (a failure even if it works)

- A calendar popup that steals keyboard focus or appears above other apps'
  windows: it must live inside the dock's layer-shell overlay like DockMenu.
- A hover animation that loops or refires while the pointer rests on an icon.
- The clock tooltip churning every second: the desklet/menu title keeps
  HH:mm and the seconds live only on the clock face itself.

## Constraints

- Qt 6.12, QML, layer-shell via LayerShellQt; the dock window is the overlay.
- Settings persist in `~/.config/kai/config.json` through `Config` with
  defaults in `config.h`; theme overrides layer on top.
- Tests are C++ (QtTest) in `tests/kai_tests.cpp`, run offscreen via
  `bin/test`; QML behavior is asserted by instantiating components with
  `QQmlComponent` (see the existing `dockMenuEntriesRenderTheirText`).
- Launcher drag-reordering must keep working with a non-launcher item at
  index 0.

## Approaches

### Calendar popup — chosen: sibling popup inside the dock window

Opened like DockMenu, positioned with `dock.anchorFor(row, …)` so it sits
above the clock on a bottom-edge dock and flips correctly for other edges.
Pros: one surface, consistent glass styling, no new layer-shell lifecycle.
Cons: positioning math must keep it inside dock bounds (same as the menu).

Rejected: separate top-level layer window — free positioning, but a second
layer-shell surface means new focus and lifecycle code for no user-visible
gain.

### Clock seconds — chosen: format extends when `clockSeconds` is on

`timeText` becomes `HH:mm:ss` when the setting is on, `HH:mm` when off.
The desklet/menu title keeps `HH:mm` so the tooltip does not churn.
Rejected: always-on seconds — it would ignore the existing setting and
punish users who turn seconds off.

### Hover animation — chosen: config value, existing animations reused

`Config` gains `hoverAnimation` (default `"bounce"`). A new Behavior page
row mirrors the click/launch/attention rows. `DockItem` plays
`animationFor(dock.v.hoverAnimation)` once on hover enter, guarded against
loop states. Rejected: a QML-only hover scale effect — it adds a second,
inconsistent motion vocabulary and no choices.

### Launcher placement — chosen: model pins `apps` at index 0

`DockModel` orders the `apps` applet before launchers, separated from the
other applets which stay right of the separator. `AppsApplet` renders the
Omarchy logo image with a fallback to the current grid glyph. Rejected:
turning it into a pinned launcher entry — that would attach window matching,
app menus and pinning semantics the launcher applet does not want.

## Left to the builder

- Popup open/close animation timing and easing.
- Exact calendar grid spacing and colors, within the existing theme tokens
  (`dock.t`), and accent from `app.accent`.
- Where the hover row sits on the Behavior page.
- File layout for the popup (`src/CalendarPopup.qml` + `resources.qrc`
  entry) and how `ClockApplet` requests it through `dock`.

## Tests

- `kai_tests`: digital `timeText` includes seconds when `clockSeconds` is on
  and not when off; `hoverAnimation` default is `bounce` and persists through
  a Config save/load round-trip; `DockModel` puts the `apps` applet first
  with launchers after it; `CalendarPopup` instantiates offscreen and shows
  the current month with today marked.
