# Clock/calendar popup, hover animation, launcher placement — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Clock applet reads HH:MM:SS and opens a month-calendar popup on click; every dock icon plays a configurable animation on hover; the apps launcher becomes dock item 0 with the Omarchy logo.

**Architecture:** All QML/C++ changes inside the existing kai tree. The calendar popup is a `QQC2 Popup` sibling of `DockMenu` inside the dock window, positioned with the same `anchorFor` math. Hover animation reuses the five existing `DockItem` animations via a new config key. `DockModel.build()` gains one front slot for the `apps` applet.

**Tech Stack:** Qt 6.12 / QML / qmake; QtTest (C++, offscreen).

**Spec:** `docs/superpowers/specs/2026-11-14-clock-calendar-hover-launcher-design.md`

## Global Constraints

- No new dependencies; Qt 6.12 only (`kai.pro` unchanged except nothing).
- Tests run via `~/Work/kai/bin/test` (qmake + `QT_QPA_PLATFORM=offscreen ./kai_tests`); all green before every commit.
- Commit messages: Conventional Commits (`feat:`, `test:`, `fix:`), one concern per commit.
- Every new `qsTr("…")` string gets a Norwegian entry in `src/i18n/no.json`; `en.json` only ever carries English plurals.
- Colors come from theme tokens (`dock.t`) and `app.accent`; no hard-coded theme colors in new UI beyond what `DockItem`/`DockMenu` already hard-code.
- No new top-level windows or layer-shell surfaces: the popup lives inside the dock window.
- Config keys are camelCase (`hoverAnimation`) with a default in `Config::defaults()`.

## Review Focus

- Pointer sweeps across many icons quickly: the hover animation must fire once per icon entered, never queue or restart mid-flight, and never fight the zoom. Pinned by `dockItemHoverPlaysOnce` and `dockItemHoverRespectsLoopState` (Task 2).
- A launch/attention loop animation is running on the icon the pointer enters: hover must not interrupt or replace the loop. Pinned by `dockItemHoverRespectsLoopState` (Task 2).
- `clockSeconds` off, and 12-hour mode with seconds: digital face must show `HH:mm` / `h:mm:ss AP` accordingly, and the desklet/menu title must never churn with seconds. Pinned by `clockDigitalSecondsFormat` (Task 5).
- The Omarchy logo file is missing (non-Omarchy systems): the grid glyph must come back. Pinned by `appsAppletLogoFallsBackToGrid` (Task 4).
- `apps` applet disabled or detached: the dock must look exactly like today (no empty front slot). Pinned by `dockModelKeepsAppsDetachedOutOfFrontSlot` (Task 3).

---

### Task 1: `hoverAnimation` setting

**Files:**
- Modify: `src/config.cpp` (`Config::defaults()`, after the `"clickAnimation"` entry)
- Modify: `src/settings/BehaviorPage.qml` (Animations section)
- Modify: `src/i18n/no.json`
- Test: `tests/kai_tests.cpp`

**Interfaces:**
- Consumes: existing `Config::defaults()`, `BehaviorPage`'s `Choice { key: …; model: page.animations }` pattern.
- Produces: config key `hoverAnimation`, default `"bounce"`, valid values `none|bounce|pulse|flip|wobble|blink` (same vocabulary as `clickAnimation`). Later tasks read `dock.v.hoverAnimation`.

- [ ] **Step 1: Write the failing test**

Add to `KaiTests`:

```cpp
void hoverAnimationDefaultAndPersist() {
    QTemporaryDir dir;
    {
        Config config(dir.filePath("cfg"), dir.filePath("data"));
        QCOMPARE(config.get("hoverAnimation").toString(), QString("bounce"));
        config.set("hoverAnimation", "flip");
    }
    Config again(dir.filePath("cfg"), dir.filePath("data"));
    QCOMPARE(again.get("hoverAnimation").toString(), QString("flip"));
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `~/Work/kai/bin/test`
Expected: FAIL — `hoverAnimation` not in defaults (`QCOMPARE` gets empty).

- [ ] **Step 3: Add the default**

In `Config::defaults()` insert, directly after the `"clickAnimation", "bounce"` line:

```cpp
{"hoverAnimation", "bounce"},
```

- [ ] **Step 4: Add the settings row**

In `BehaviorPage.qml`, in the Animations section between the "On click" row and the "While an app starts" row:

```qml
SettingRow {
    title: qsTr("On hover")
    Choice { key: "hoverAnimation"; model: page.animations }
}
```

- [ ] **Step 5: Add the Norwegian string**

In `src/i18n/no.json`, next to `"On click"`, add:

```json
"On hover": "Når pekeren er over",
```

- [ ] **Step 6: Run tests to verify they pass**

Run: `~/Work/kai/bin/test`
Expected: PASS (including the existing `bundledCatalogsAreValid`).

- [ ] **Step 7: Commit**

```bash
cd ~/Work/kai && git add src/config.cpp src/settings/BehaviorPage.qml src/i18n/no.json tests/kai_tests.cpp
git commit -m "feat: hover animation setting for dock icons"
```

### Task 2: DockItem plays the hover animation

**Files:**
- Modify: `src/DockItem.qml` (add `playHover()` next to `playClick()` at ~line 282)
- Modify: `src/DockWindow.qml` (`band.onPositionChanged`, ~line 287–289)
- Test: `tests/kai_tests.cpp`

**Interfaces:**
- Consumes: `DockItem.play(name, loops)`, `DockItem.loopState`, `dock.v.hoverAnimation` (Task 1).
- Produces: `DockItem.playHover()` — plays the hover animation once, no-op while a loop state (`launching`/`urgent`) is active or an animation is already running. `DockWindow.band.onPositionChanged` calls it whenever the icon under the pointer changes.

- [ ] **Step 1: Write the failing tests**

Add a stub dock usable by `DockItem` in the test file (a QML stub created through `QQmlComponent`, as a sibling of `MenuAppStub`):

```qml
// QtObject with: s, t, geom {u,w,scale,start}, iconBase, frameRotation,
// zoomFactor, panel, dragging, dragIndex, speed, gap, pad, iconBase —
// every property DockItem reads at creation: s=48, frameRotation=0,
// zoomFactor=1, iconBase=10, speed=1, dragging=false, geom = {u:[0], w:[48], scale:[1], start:0}
```

Then:

```cpp
void dockItemHoverPlaysOnce() {
    // create window + DockItem(kind "app") inside it with the stub dock
    // call delegate.playHover(); QTest::qWait(300);
    // assert delegate.property("bounce").toDouble() > 0
}
void dockItemHoverRespectsLoopState() {
    // same setup, but item launched=true (loopState "launch")
    // playHover(); qWait(300); assert bounce == 0
}
```

(Write these with the same QQmlComponent + `createWithInitialProperties` pattern as `dockMenuEntriesRenderTheirText`; the DockItem required properties are `dock, app, index, kind, appId, name, icon, appletId, pinned, running, windowCount, active, urgent, launching`.)

- [ ] **Step 2: Run tests to verify they fail**

Run: `~/Work/kai/bin/test`
Expected: FAIL — `playHover` unknown method.

- [ ] **Step 3: Implement `playHover()` in `DockItem.qml`**

Next to `playClick()`:

```qml
function playHover() {
    if (loopState !== "") return
    if (bounceAnim.running || pulseAnim.running || flipAnim.running || wobbleAnim.running || blinkAnim.running)
        return
    play(dock.v.hoverAnimation, 1)
}
```

- [ ] **Step 4: Wire the trigger in `DockWindow.qml`**

Replace `win.hoveredIndex = indexAt(mouse.x)` in `band.onPositionChanged` with:

```qml
const i = indexAt(mouse.x)
if (i !== win.hoveredIndex) {
    const d = i >= 0 ? items.itemAt(i) : null
    if (d && d.kind !== "separator" && !win.dragging) d.playHover()
    win.hoveredIndex = i
}
```

(`d.kind` is exposed because the delegate's required `kind` property is readable as an item property.)

- [ ] **Step 5: Run tests to verify they pass**

Run: `~/Work/kai/bin/test`
Expected: PASS.

- [ ] **Step 6: Commit**

```bash
cd ~/Work/kai && git add src/DockItem.qml src/DockWindow.qml tests/kai_tests.cpp
git commit -m "feat: play the configured hover animation when the pointer enters an icon"
```

### Task 3: DockModel puts the apps applet first

**Files:**
- Modify: `src/dockmodel.cpp` (`build()`, the `result` assembly at ~line 152–168)
- Test: `tests/kai_tests.cpp`

**Interfaces:**
- Consumes: `m_config->get("applets")` entries (`enabled`, `detached`, `id`).
- Produces: when the `apps` applet is enabled and not detached, `build()` emits it as row 0 (`kind "applet"`, `key "applet:apps"`), flush before all launchers, with no separator between it and the first launcher; all other applets stay after `sep:applets`. When it is disabled or detached, output is identical to today.

- [ ] **Step 1: Rewrite the failing assertions in `dockModelOrdersItems`**

Update the existing test so the expected sequence is:

```
0: kind "applet", appletId "apps"
1: app "b" (pinned)
2: app "a" (pinned)
3: kind "separator" (key "sep:applets")
4..6: applets clock, sysmon, weather   (apps no longer among them)
```

and add `dockModelKeepsAppsDetachedOutOfFrontSlot`: set `config.setApplet("apps", "detached", true)`, rebuild, assert `items[0].appId == "b"` and no item with `appletId == "apps"` exists.

- [ ] **Step 2: Run tests to verify they fail**

Run: `~/Work/kai/bin/test`
Expected: FAIL — `items[0]` is still app `b`.

- [ ] **Step 3: Implement in `build()`**

In the applet-collecting loop, capture the `apps` entry separately; in the `result` assembly, prepend it before `result << out.mid(0, pinnedCount)` and skip it from the trailing `applets` list:

```cpp
DockItem appsFront;                     // found while looping applets
bool hasAppsFront = false;
// in the loop: if id == "apps" → appsFront = item; hasAppsFront = true; continue;
…
if (hasAppsFront) result << appsFront;  // no separator after it
result << out.mid(0, pinnedCount);
```

Leave the `sep:running` and `sep:applets` conditions unchanged.

- [ ] **Step 4: Run tests to verify they pass**

Run: `~/Work/kai/bin/test`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
cd ~/Work/kai && git add src/dockmodel.cpp tests/kai_tests.cpp
git commit -m "feat: the apps launcher is dock item 0, flush before the launchers"
```

### Task 4: Omarchy logo on the apps applet

**Files:**
- Modify: `src/applets/AppsApplet.qml`
- Test: `tests/kai_tests.cpp`

**Interfaces:**
- Consumes: nothing new.
- Produces: `AppsApplet.logoSource` (string, default `"file:///usr/share/omarchy/icon.png"`); a full-size `Image` (preserve aspect, `status === Image.Ready` → visible) replacing the grid glyph; the grid glyph comes back whenever the image is not `Ready`. The logo file on this machine is a 300×300 PNG on transparency (green/yellow mark) and needs no tinting.

- [ ] **Step 1: Write the failing tests**

```cpp
void appsAppletShowsOmarchyLogo() {
    // QQmlComponent load qrc:/applets/AppsApplet.qml with app stub;
    // find the Image child; assert source contains "omarchy/icon.png"
}
void appsAppletLogoFallsBackToGrid() {
    // same, but set logoSource = "file:///nonexistent/kai-missing.png" via
    // initial property; assert the Grid glyph child is visible and the Image is not
}
```

- [ ] **Step 2: Run tests to verify they fail**

Run: `~/Work/kai/bin/test`
Expected: FAIL — no Image child.

- [ ] **Step 3: Implement**

In `AppsApplet.qml`, add `property string logoSource: "file:///usr/share/omarchy/icon.png"` and wrap the grid in `visible: logo.status !== Image.Ready`, adding:

```qml
Image {
    id: logo
    anchors.fill: parent
    anchors.margins: parent.width * 0.14
    source: applet.logoSource
    sourceSize: Qt.size(256, 256)
    fillMode: Image.PreserveAspectFit
    smooth: true
    visible: status === Image.Ready
}
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `~/Work/kai/bin/test`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
cd ~/Work/kai && git add src/applets/AppsApplet.qml tests/kai_tests.cpp
git commit -m "feat: the launcher applet carries the Omarchy logo"
```

### Task 5: Clock shows HH:MM:SS

**Files:**
- Modify: `src/applets/ClockApplet.qml` (`timeText`, `title` at ~lines 23–31)
- Test: `tests/kai_tests.cpp`

**Interfaces:**
- Consumes: config keys `clockSeconds`, `clock24h`, `clockAnalog`; context properties `config`, `translator` (as in the app).
- Produces: `timeText` includes `:ss` when `clockSeconds` is on (`HH:mm:ss` / `h:mm:ss AP`), plain `HH:mm` / `h:mm AP` when off; `title` (desklet/menu text) keeps minutes only, built from a new `titleTime` property.

- [ ] **Step 1: Write the failing test**

```cpp
void clockDigitalSecondsFormat() {
    // context properties: Config(tempdir) as "config", Translator as "translator",
    // MenuAppStub as "app"; QQmlComponent load qrc:/applets/ClockApplet.qml
    // clockSeconds true → item property timeText matches QRegularExpression("\\d{1,2}:\\d{2}:\\d{2}")
    //                  → title does NOT match that pattern
    // clockSeconds false → timeText matches QRegularExpression("\\d{1,2}:\\d{2}$")
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `~/Work/kai/bin/test`
Expected: FAIL — no seconds in `timeText`.

- [ ] **Step 3: Implement**

```qml
readonly property string timeText: {
    translator.language
    const base = applet.h24 ? "HH:mm" : "h:mm AP"
    return applet.now.toLocaleTimeString(Qt.locale(), applet.seconds ? base + ":ss" : base)
}
readonly property string titleTime: {
    translator.language
    return applet.now.toLocaleTimeString(Qt.locale(), applet.h24 ? "HH:mm" : "h:mm AP")
}
```

and `title` becomes `titleTime + " · " + dateText`. (`timeText` consumers: the digital Column and the desklet's second Column — both keep reading `timeText`.)

- [ ] **Step 4: Run test to verify it passes**

Run: `~/Work/kai/bin/test`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
cd ~/Work/kai && git add src/applets/ClockApplet.qml tests/kai_tests.cpp
git commit -m "feat: the digital clock face shows seconds"
```

### Task 6: CalendarPopup component

**Files:**
- Create: `src/CalendarPopup.qml`
- Modify: `src/resources.qrc` (add `<file>CalendarPopup.qml</file>` next to `DockMenu.qml`)
- Test: `tests/kai_tests.cpp`

**Interfaces:**
- Consumes: `app.radius` / `app.accent` (stub in tests), `config.values` (only for nothing initially — colors come from the `app` stub and hard constants matching DockMenu).
- Produces: `CalendarPopup` — a `Popup` with `required property var dock`, `required property var app`, `property int row: -1`, `property date viewMonth`, `readonly property string monthLabel`, `function openFor(index)`, `function shift(delta)` (±1 month). `openFor` positions with the same math as `DockMenu.place()` using `dock.anchorFor(index, 6)` and calls `open()`.

- [ ] **Step 1: Write the failing test**

```cpp
void calendarPopupShowsMonthAndNavigates() {
    // ApplicationWindow in the test; Popup parent = window.contentItem;
    // stub dock object with anchorFor() → Qt.point(100, 100), width/height large
    // createWithInitialProperties({ dock: stub, app: stub, parent: window.contentItem })
    // invoke openFor(0) → isOpen true; read monthLabel
    // invoke shift(1) → monthLabel differs from before
    // invoke shift(-2) → monthLabel differs again
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `~/Work/kai/bin/test`
Expected: FAIL — component does not exist.

- [ ] **Step 3: Implement `CalendarPopup.qml`**

Skeleton (bodies of the day-offset math pinned here, styling left to the implementer within the file's own conventions):

```qml
Popup {
    id: popup
    required property var dock
    required property var app
    property int row: -1
    property date viewMonth: new Date(new Date().getFullYear(), new Date().getMonth(), 1)
    readonly property string monthLabel:
        popup.viewMonth.toLocaleDateString(Qt.locale(), "MMMM yyyy")
    width: 280
    padding: 12
    modal: false
    dim: false
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

    function openFor(index) {
        row = index
        // same place() math as DockMenu: anchor above (bottom edge) / beside (other edges),
        // clamped to dock.width/height with a 6px margin, then open()
    }
    function shift(delta) {
        viewMonth = new Date(viewMonth.getFullYear(), viewMonth.getMonth() + delta, 1)
    }
    // contentItem: Column { header Row [◀ monthLabel ▶] ; Grid columns: 7 with
    // weekday header cells from Qt.locale().firstDayOfWeek, then
    // leading blanks = (firstDayOfWeekOffsetOfFirst + 6) % 7 … day cells 1..daysInMonth,
    // today = accent-filled rounded cell }
}
```

`daysInMonth`: `new Date(y, m + 1, 0).getDate()`; first-day offset: `(new Date(y, m, 1).getDay() - Qt.locale().firstDayOfWeek + 7) % 7` where `Qt.locale().firstDayOfWeek` is 1 (Mon)…7 (Sun) and `Date.getDay()` is 0 (Sun)…6 (Sat).

- [ ] **Step 4: Run test to verify it passes**

Run: `~/Work/kai/bin/test`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
cd ~/Work/kai && git add src/CalendarPopup.qml src/resources.qrc tests/kai_tests.cpp
git commit -m "feat: month calendar popup component"
```

### Task 7: Clock click opens the popup; gnome-calendar stays reachable

**Files:**
- Modify: `src/DockWindow.qml` (instantiate the popup next to `DockMenu`, ~line 380; `openCalendar`/`calendarClosed` functions; `surfaceDepth` and `updateMask` treat `popupOpen` like `menuOpen`)
- Modify: `src/DockItem.qml` (`activateApplet(button)` → `activateApplet(button, row)`, passing its `index` through)
- Modify: `src/AppletView.qml` (`activate(button, row)`)
- Modify: `src/applets/ClockApplet.qml` (`activate`, plus a `menuEntries()` function)
- Modify: `src/DockWindow.qml` `band.onClicked` (applet branch passes the row)
- Modify: `src/i18n/no.json`
- Test: `tests/kai_tests.cpp`

**Interfaces:**
- Consumes: Task 6 `CalendarPopup.openFor(index)`; `shell.hasCommand/run` (context property, unchanged).
- Produces: `DockWindow.openCalendar(index)` sets `popupOpen = true` and opens the popup; `calendarClosed()` clears it and runs the same tail as `menuClosed()`. `ClockApplet.activate(button, row)`: dock mode → `dock.openCalendar(row)`; desklet mode or no dock → gnome-calendar launch exactly as today. `ClockApplet.menuEntries()` → `[{ text: qsTr("Open calendar app…"), action: () => shell.run("gnome-calendar") }]` (DockMenu already renders applet `menuEntries()`).

- [ ] **Step 1: Write the failing test**

```cpp
void clockClickOpensCalendarPopupInDockMode() {
    // stub app object gains a spy: openCalendarCalls (int)
    // stub dock object exposes openCalendar(index) { openCalendarCalls++ }
    // context: config, translator, app, shell stub (hasCommand→false, run no-op)
    // load qrc:/applets/ClockApplet.qml with initial property dock = stubDock
    // invoke activate(0, 3) → openCalendarCalls == 1
    // set mode "desklet" (dock stays but mode decides) → invoke activate(0, 3)
    //   → openCalendarCalls still 1  (desklet keeps gnome-calendar)
    // invoke menuEntries() → QVariantList with ≥1 entry, first text non-empty
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `~/Work/kai/bin/test`
Expected: FAIL — `activate` ignores the row / no `menuEntries`.

- [ ] **Step 3: Implement the applet side**

In `ClockApplet.qml`:

```qml
property var dock
function activate(button, row) {
    if (mode === "dock" && dock) { dock.openCalendar(row); return }
    if (shell.hasCommand("gnome-calendar")) shell.run("gnome-calendar")
}
function menuEntries() {
    return [{ text: qsTr("Open calendar app…"), action: () => shell.run("gnome-calendar") }]
}
```

In `AppletView.qml`: `function activate(button, row) { if (item && item.activate) item.activate(button, row) }`.
In `DockItem.qml`: `function activateApplet(button, row) { playClick(); appletView.activate(button, row) }` (`row` defaults to `-1` when QML calls omit it).
In `DockWindow.qml` applet click branch: `delegate.activateApplet(mouse.button, i)`.

- [ ] **Step 4: Implement the dock-window side**

In `DockWindow.qml`, after the `DockMenu` instantiation:

```qml
property bool popupOpen: false
CalendarPopup { id: calendar; dock: win; app: win.app; parent: overlay; onClosed: dock.calendarClosed() }
function openCalendar(index) {
    popupOpen = true
    Qt.callLater(() => calendar.openFor(index))
}
function calendarClosed() {
    popupOpen = false
    if (!band.containsMouse) { zoomAmount = 0; hoveredIndex = -1 }
    Qt.callLater(updateMask)
}
```

`surfaceDepth` becomes `(menuOpen || popupOpen) ? Math.max(depth, vertical ? 420 : 560) : depth` and `updateMask()`'s first branch checks `menuOpen || popupOpen`.

- [ ] **Step 5: Add the Norwegian string**

In `src/i18n/no.json` next to `"Applet settings…"`:

```json
"Open calendar app…": "Åpne kalenderapp …",
```

- [ ] **Step 6: Run tests to verify they pass**

Run: `~/Work/kai/bin/test`
Expected: PASS (all, including `bundledCatalogsAreValid`).

- [ ] **Step 7: Commit**

```bash
cd ~/Work/kai && git add src/DockWindow.qml src/DockItem.qml src/AppletView.qml src/applets/ClockApplet.qml src/i18n/no.json tests/kai_tests.cpp
git commit -m "feat: clicking the clock opens the calendar popup; calendar app stays in the menu"
```

### Task 8: Full verification, rebuild, restart

**Files:** none (verification only).

**Interfaces:**
- Consumes: everything above.

- [ ] **Step 1: Run the whole test suite**

Run: `~/Work/kai/bin/test`
Expected: all PASS, 0 failed.

- [ ] **Step 2: Rebuild the dock**

Run: `~/Work/kai/bin/build`
Expected: `Built /home/rune/Work/kai/build/kai`.

- [ ] **Step 3: Restart the running dock**

```bash
pkill -x kai; sleep 1; ~/Work/kai/build/kai &
```

- [ ] **Step 4: Commit any straggler and report**

If `git status` is clean, done. Report to Rune for the visual pass: hover bounce on icons, "On hover" row in Behavior settings, launcher logo at the far left, HH:MM:SS clock, calendar popup on click, «Åpne kalenderapp …» in the clock's right-click menu.
