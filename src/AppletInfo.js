.pragma library

// Names and descriptions of the applets, for menus and Settings.
function name(id) {
    switch (id) {
    case "apps": return qsTr("Applications")
    case "clock": return qsTr("Clock")
    case "weather": return qsTr("Weather")
    case "sysmon": return qsTr("System monitor")
    case "battery": return qsTr("Battery")
    case "trash": return qsTr("Trash")
    case "windowskiller": return qsTr("WindowsKiller")
    case "volume": return qsTr("Volume")
    }
    return id
}

function description(id) {
    switch (id) {
    case "apps": return qsTr("Opens the app launcher.")
    case "clock": return qsTr("Analog or digital time and date.")
    case "weather": return qsTr("Current weather and a three-day forecast from Open-Meteo.")
    case "sysmon": return qsTr("CPU and memory load, with a history graph.")
    case "battery": return qsTr("Charge level and whether it's charging.")
    case "trash": return qsTr("Shows how much is in the trash; opens or empties it.")
    case "volume": return qsTr("Scroll to change the volume, click to mute, double click for the mixer.")
    case "windowskiller": return qsTr("Left click, then pick a stuck window to kill it. Middle click shows its pid, command, uptime and class. Pick the dock or press Esc to cancel.")
    }
    return ""
}
