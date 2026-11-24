#pragma once
// What MK3D's online client (ServerClient, UdpRendezvous, MatchController) reads from its launcher's settings and
// writes to its log, answered from this app's Settings.
#include <QString>
#include <QStringList>

namespace paths {
QString dataDir();      // the app's data folder (%APPDATA%\MK Arcade Online)
QString sessionsDir();  // one folder per match game: its game.log (desync and crash reports read it)
}  // namespace paths

namespace cfg {
QString serverAddress();
QString fingerprint();
QString gameFolder(const QString& gameId);
QString gameExe(const QString& gameId);  // the game runs inside this app: a name only
QString region();
int maxPing();
bool strict();
bool practiceWhileWaiting();  // no: one game at a time, the match's
bool searchAgain();
int inputDelay();
bool autoDelay();
int gamePort();
int localTestPort(int instance);
bool udpLoopback();
bool udpBlocked();
bool hideIp();
bool hideIpAlways();
}  // namespace cfg

namespace applog {
void write(const QString& line);  // a timestamped line in online.log
}
