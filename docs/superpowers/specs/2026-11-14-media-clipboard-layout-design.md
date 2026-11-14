# Digital clock, media and clipboard applets, and dock layout

## Intent

Make kai more useful without losing its Cairo-Dock character. The clock will show only a 24-hour `HH:mm:ss`. New media and clipboard applets will use bounded popups rather than crowding the dock. The dock will get stable applet placement, clearer state indicators, calmer applet zoom, consistent applet tiles, explicit overflow handling, and useful window labels.

## Goals

- Show `HH:mm:ss` in the digital clock, with leading zeroes and no date or AM/PM.
- Control MPRIS-compatible players without requiring `playerctl`.
- Search, preview, pin, and restore cliphist entries, including images.
- Keep docked applets stationary when running apps appear or disappear.
- Keep every docked applet reachable at small screen widths and with many apps.
- Preserve pinned-app drag ordering, app-window cycling, all four screen edges, desklets, themes, and the existing renderers.
- Ship Norwegian and English UI text and automated coverage for parsing, ordering, and fallback behavior.

## Non-goals

- A workspace applet, notification applet, or other suggestions outside the selected scope.
- A new clipboard database or background clipboard collector inside kai.
- Automatic package installation. Missing clipboard tools produce an actionable empty state.
- A theme-engine rewrite or removal of reflections, gloss, and existing indicator themes.
- Keeping analog-clock or 12-hour-clock options.

## Anti-goals

- Do not move the applet zone as the set of running apps changes.
- Do not hide applets to make room for apps. If the applets alone physically exceed the screen, keep them reachable through the explicit utility-overflow rule below.
- Do not log clipboard contents or persist decoded image thumbnails beyond the runtime session.
- Do not use shell pipelines for clipboard data or media metadata.
- Do not let app magnification overlap or displace the applet zone.
- Do not break kai when D-Bus, MPRIS players, `cliphist`, or `wl-copy` are unavailable.

## Constraints and current state

- The project is C++17, Qt 6.12, QML, qmake, and LayerShellQt.
- `ClockApplet.qml` already has a seconds timer, but digital text ignores `clockSeconds` and always adds a date line.
- `DockModel::build()` already orders pinned apps, running apps, separators, and applets. `DockWindow.qml` centers the entire row, so all items move when its length changes.
- The dock surface has no keyboard focus and layer-shell keyboard interactivity is `None`. Clipboard search therefore cannot be implemented as a `TextField` inside the current dock surface.
- `cliphist` and `playerctl` are not installed on the development machine. `wl-copy` and `wl-paste` are installed.
- Qt D-Bus supplies MPRIS integration. cliphist supplies clipboard history through `<ID>\t<PREVIEW>` records; `decode` and `delete` accept selected records without shell interpolation.
- The working tree was clean when this design was written.

## Approaches

### Selected: MPRIS over Qt D-Bus and cliphist through QProcess

Use a process-wide media controller and clipboard controller exposed to QML. MPRIS properties and signals drive media state. Clipboard commands run asynchronously only when the clipboard popup needs data or performs an action. A shared popup window provides keyboard focus and consistent placement.

This avoids a `playerctl` dependency and media polling. It reuses the user's existing cliphist history instead of creating a second history store.

### Rejected: `playerctl` and cliphist through shell commands

This needs less D-Bus code, but adds a dependency and requires polling for playback state and progress. Shell pipelines also make binary clipboard handling and argument safety easier to get wrong.

### Rejected: kai-owned clipboard capture through `wl-paste --watch`

This would avoid cliphist, but requires a persistent watcher, a private history format, image storage, deduplication, and retention behavior. It duplicates cliphist and adds background access to clipboard contents.

## Design

### 1. Digital-only clock

Replace the analog/digital clock implementation with one digital renderer for dock and desklet:

- Format with `Qt.formatTime(now, "HH:mm:ss")` so output is always zero-padded, 24-hour, eight characters, and independent of locale conventions.
- Remove the date from the dock, desklet, and hover title. The calendar click action may remain.
- Use tabular numerals and update once per second.
- Use a compact desklet of roughly 230 × 100 logical pixels; final sizing is a builder detail.
- Remove the Analog, 24-hour, and Seconds settings. Old stored keys are ignored and require no destructive config migration.
- Give the dock clock a wide tile, about 1.6 icon widths, so eight digits remain legible. Other applets remain square. All tiles share height, radius rules, border, and padding.
- Update applet descriptions and translations.

### 2. Shared applet popup

Add a single reusable popup surface owned by `Main.qml` and opened with an applet ID plus a global anchor rectangle.

- Use a separate LayerShellQt window, not the non-focused dock window.
- Give it zero exclusive zone and on-demand keyboard interactivity. Use an ordinary frameless window off Wayland for tests.
- Position it beside the invoking dock icon or desklet on bottom, top, left, and right edges. Clamp it to the available screen.
- Match the active theme's background, border, radius, text, and accent.
- Close on Escape, the close button, deactivation/outside dismissal, or a completed clipboard restore. Reopening the same applet toggles it; opening another applet replaces its content.
- Keep the popup bounded: media uses one compact card; clipboard uses at most about 60% of screen height with an internal scroll view.
- Restore dock input masks and hover state when it closes. Never leave an invisible input-grabbing region behind.

The exact LayerShellQt helper names and anchor calculations are left to the builder, but keyboard entry must work in a real Wayland session.

### 3. Media applet and MPRIS controller

Add `media` to the known applets, resources, settings, and translations. New installations show it by default; upgrades add it disabled so an update does not unexpectedly alter an existing dock.

The process-wide controller will:

- Discover `org.mpris.MediaPlayer2.*` names and react to players appearing and disappearing.
- Prefer a playing player, then a paused player, then the player that most recently had playback activity. The popup offers a player selector when several exist. An explicit user selection overrides automatic ranking until that player disappears or the user chooses Automatic; then ranking resumes.
- Read playback status, metadata, volume, capabilities, track length, and position through MPRIS properties and `PropertiesChanged` signals.
- Expose track title, artists, album, art URL, playback state, duration, volume, and available actions to QML.
- Invoke PlayPause, Previous, Next, Seek/SetPosition, and Volume only when supported. Update position with a lightweight timer only while the popup is visible; do not poll all players continuously.

The dock tile shows album art when available and a themed media glyph otherwise. A small playing/paused state mark is allowed, but no track text is squeezed into the tile.

Interactions:

- Left-click: play/pause. This remains useful without opening the popup.
- Mouse wheel over the applet: adjust the selected player's volume in bounded steps.
- Right-click menu: open media controls, previous, play/pause, next, and applet options.
- Popup: artwork, title/artist, previous/play/next, elapsed and total time, seek slider when supported, volume, and player selection.

No player state is fabricated when no player or session bus exists; show a quiet unavailable state.

The media desklet is a roughly 320 × 150 compact status card with artwork, title, artist, and playing/paused state. Left-click still plays/pauses. Its right-click applet menu opens the full controls popup beside the desklet, in addition to previous/next actions. The desklet itself does not need nested sliders or buttons.

### 4. Clipboard applet and cliphist controller

Add `clipboard` to the known applets, resources, settings, and translations. New installations show it by default; upgrades add it disabled.

The process-wide controller will:

- Run `cliphist list` asynchronously when the popup opens and after pin/copy actions that need refreshed state.
- Parse the first tab as an unsigned numeric ID and retain the rest as the preview. Never split on later tabs or execute preview text.
- Keep search across the loaded records case-insensitive. Show pinned matches first, then newest matches. Limit rendered rows while retaining search across all loaded records.
- Store only pinned cliphist IDs in kai config. Ignore stale IDs that no longer occur in cliphist; do not store clipboard text or images in kai config.
- Provide pin/unpin and delete actions. Delete passes the exact selected record to `cliphist delete` over stdin and refreshes on success.
- Restore an entry by connecting `cliphist decode` directly to `wl-copy` with `QProcess` process chaining. Do not invoke `/bin/sh` or route binary data through QML.
- Decode visible image entries asynchronously for thumbnails. Keep thumbnails in a private runtime location with restrictive permissions, remove them when replaced or on shutdown, and bound their size/count. Show image metadata when decoding fails.
- Show empty, loading, error, missing-`cliphist`, and missing-`wl-copy` states in the popup.

The dock tile uses the shared applet tile and a clipboard glyph. It does not continuously read or display clipboard contents. The clipboard desklet is a small static card with a glyph and an Open history action; it never shows clipboard previews on the desktop. Left-click opens the same focused popup beside the desklet.

Interactions:

- Left-click: open the bounded history popup and focus Search.
- Popup row click: restore that entry and close.
- Row actions: pin/unpin and delete.
- Right-click menu: open history, refresh, and applet options.

### 5. Stable dock zones and magnification

Keep the model's logical order:

```text
[pinned apps] | [running apps] | [applets]
```

Change geometry rather than changing the user's launcher ordering:

- Compute a fixed applet end position from the no-running-apps resting layout. Applets keep these resting centers when running apps come and go.
- Grow and shrink the app area toward the opposite side. Preserve a centered resting layout when no overflow exists.
- Handle floating, panel, and 3D-plane shelves in logical along-edge coordinates so the existing rotations continue to work.
- Give each cell a width factor; only the digital clock initially uses a non-square factor.
- Calculate app zoom inside the app zone. Keep the existing pointer-fixed parabolic behavior there.
- Calculate applet zoom independently at half strength: `1 + (normalZoom - 1) × 0.5`. Applets scale around their own centers and do not push neighbors.
- Clip app visuals at the zone boundary and keep separators aligned with their zones.
- Include wide cells and both zoom zones in hover hit testing, drag ordering, labels, menu anchors, reflections, shelf bounds, and input masks.

For a full-width panel, the applet zone is fixed to the logical end of the panel. For floating and plane views, the resting composition remains compact rather than becoming full width.

### 6. Running versus active indicators

Keep the configured running-indicator style from the theme:

- Running apps retain dots, bar, or glow as configured.
- The focused app gets an additional short, brighter active line using `activeColor`.
- An inactive running app uses reduced opacity and no active line.
- Multiple-window dots still cap at three.

The active line must remain visible on floating, panel, and plane themes without moving the icon.

### 7. Consistent applet tiles

Use `AppletTile.qml` for every docked applet, including Trash and the new applets. Standardize height, margins, radius calculation, border, and content padding. Existing semantic tints and glyphs may remain.

Desklets retain their existing card layout; this consistency rule applies to dock tiles. The clock is the only intentionally wide tile.

### 8. Overflow with a More menu

Set a maximum dock length from the available screen length and safe edge padding.

- Never hide the applet zone.
- Fill the app zone in model order. Put trailing app items that do not fit into an overflow set.
- Show a dedicated More cell immediately before the applet separator whenever the overflow set is nonempty.
- Open a menu with hidden app names and icons. Activating an entry performs the normal app action; its existing window/pin actions remain available where practical.
- Keep the More cell in geometry, hit testing, labels, shelf bounds, and input masks.
- Hidden items have no clickable or draggable region. Hidden pinned apps can still be reordered in Settings.
- Recompute on model, zoom, icon-size, screen, and configuration changes. The bottom and right edges must not clip as in the current 3D-plane example.
- If the applet zone alone exceeds the available length, first reduce only its tile scale uniformly, down to a minimum of 24 logical pixels. If it still does not fit, add a utility-overflow cell inside that zone and put the trailing applets in its popup menu. Applet actions remain available from this menu. This extreme case must not push content off-screen or reduce app tiles.

The builder may implement overflow as a virtual QML cell or model support; the external behavior is fixed. Desklet right-click menus and popup invocations use the same applet actions as dock menus, anchored to the desklet rather than the dock.

### 9. Useful fallback labels

For a running window that cannot be matched to a desktop entry, use its nonempty window title as the dock label; otherwise use its window class. This removes the current unmatched-window behavior that always chooses the class, including labels such as `GTK Application` when a useful title exists. Matched desktop entries keep their desktop-file names.

### 10. Settings, resources, and documentation

- Register new QML/C++ files in qmake and Qt resources.
- Add `QT += dbus`; no `playerctl` dependency.
- Document cliphist and `wl-copy` as clipboard-applet requirements, not requirements for starting kai.
- Update README applet lists and interactions.
- Add Norwegian translations and English plural forms where needed.
- Do not edit the user's live `~/.config/kai` configuration as part of implementation.

## Verification

Automated:

- Existing Qt tests remain green.
- Config tests cover new applet registration and upgrade behavior.
- Dock-model tests cover zone ordering, More-menu candidates where model support is used, and title/class fallback.
- Media tests cover player ranking and MPRIS metadata/capability parsing without requiring a real player.
- Clipboard tests cover numeric IDs, previews containing tabs, text versus image records, stale pinned IDs, and missing-tool failures without reading the user's real history.
- Clock coverage verifies examples such as midnight `00:00:00` and non-midnight `09:07:05`.
- Geometry tests or deterministic QML checks cover wide clock cells, stationary applet centers, half zoom, and overflow at narrow widths.
- Run `bin/build` and `bin/test` with the offscreen test setup.

Manual Wayland checks:

- Spotify or another MPRIS player: appear/disappear, player switching, play/pause, previous/next, volume, art, and supported/unsupported seeking.
- cliphist: text, tabs, image preview/restore, search keyboard focus, pin persistence, delete, empty history, and missing tools.
- Digital clock in dock and desklet, including leading zeroes and no date.
- Floating and plane views on the bottom edge; one vertical edge; panel view; narrow screen/many apps; zoom and More-menu interaction.
- Running/active indicators, app drag ordering, app-window scroll cycling, popup dismissal, and input-mask cleanup.

## Builder discretion

Internal class names, helper decomposition, model-versus-QML overflow representation, exact popup dimensions, icon glyph sources, thumbnail limits, and test-fake mechanics are builder choices. They must not change the behavior above.
