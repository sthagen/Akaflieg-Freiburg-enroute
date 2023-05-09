# iOS

## Known issues:

- [X] Keyboard überdeckt UI (überall)
- [ ] Manche map tiles sind manchmal leer
- [ ] Beim Laden von Flugzeug aus Lib wird name nicht übernommen
- [ ] Crash beim Start wenn App lange inaktiv war (Zugriff auf Speicher)
- [X] Map zeigt manche Linien nicht an
- [ ] Safe area funktioniert manchmal manchmal nicht (glaube wenn App im Hintergrund war)
- [X] Höhe AMSL stimmt nicht
- [ ] Xcode warnt vor Memory leaks (verschiedene Stellen)


## Not yet implemented

- [ ] Notifications (noch nicht fertig)
- [ ] Import und Export
- [ ] Kennwörter (SSID) -> sieht schwierig aus unter iOS. Viele Changes in den letzen iOS-Versionen, von Apple nicht gern gesehen
- [ ] Automatische Verbindung mit Verkehrsdatenempfänger


# General

- [ ] Generate spritesheet in proper way
- [ ] Update manual: onboarding, NOTAMs
- [ ] Remove crazy large GIT LTS files from GitHub.


## Bug Fixing

- [ ] Fix problems with auto-generated screenshots for google play
- [ ] Weather: Indicator for "Downloading Stations" does not work
- [x] GUI: Aircraft library: layout goes to edge of screen
- [x] GUI: Aircraft library: clear text button does not work
- [x] GUI: Save Aircraft dialog: dialog does not repond to virtual keyboard
- [x] Color mismatch: "teal" vs Material.Teal
- [x] Settings page: Switch "Night Mode" on/off, get color mismatch in title bar
- [x] Set proper title bar for "Waypoint", "Notam" and "Weather" dialogs, will be important with Material3
- [x] Route & Wind: Click 'clear' icon in Wind direction gives 360°, but should give invalid


## Minor GUI improvements

- [x] Settings page: oberer freigehaltener Rand sollte im Scrollview sein, schaut besser aus
- [x] Lebensdauer GPS Fix für Desktop-Geräte auf sehr großen Wert erhöhen


## New Features

- [ ] Indicator that maps are being loaded
- [ ] METAR/TAF rewrite
- [ ] Exit app on double 'back'
- [ ] https://www.qt.io/blog/qt-for-android-storage-updates
- [ ] https://www.qt.io/blog/deploying-to-linux-with-cmake
