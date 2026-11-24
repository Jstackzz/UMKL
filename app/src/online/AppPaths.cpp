#include "AppPaths.h"

#include "../Settings.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>

namespace {
Settings* S() { return Settings::instance(); }
}  // namespace

namespace paths {
QString dataDir() { return S()->dataDir(); }
QString sessionsDir()
{
    const QString d = dataDir() + "/sessions";
    QDir().mkpath(d);
    return d;
}
}  // namespace paths

namespace cfg {
QString serverAddress() { return S()->server(); }
QString fingerprint() { return S()->fingerprint(); }
QString gameFolder(const QString& gameId) { return S()->romDir() + "/" + gameId; }
QString gameExe(const QString& gameId) { return QStringLiteral("embedded:") + gameId; }
QString region() { return S()->region(); }
int maxPing() { return S()->maxPing(); }
bool strict() { return false; }
bool practiceWhileWaiting() { return false; }
bool searchAgain() { return S()->searchAgain(); }
int inputDelay() { return qBound(0, S()->inputDelay(), 4); }
bool autoDelay() { return true; }
int gamePort() { return S()->netPort(); }
int localTestPort(int instance) { return S()->netPort() + instance; }
bool udpLoopback() { return S()->loopbackOnly(); }
bool udpBlocked() { return false; }
bool hideIp() { return S()->hideIp(); }
bool hideIpAlways() { return false; }
}  // namespace cfg

namespace applog {
void write(const QString& line)
{
    static QMutex lock;
    QMutexLocker locker(&lock);
    const QString text = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz ")) + line;
    QFile f(paths::dataDir() + "/online.log");
    if (f.open(QIODevice::Append | QIODevice::Text))
        f.write(text.toUtf8() + '\n');
    qInfo().noquote() << text;
}
}  // namespace applog
