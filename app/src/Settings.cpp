#include "Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

static Settings* g_instance = nullptr;

static QString DataDir()
{
    // tests: MKA_DATA_DIR gives a copy its own settings, account and sessions (two copies on one PC)
    QString d = qEnvironmentVariable("MKA_DATA_DIR");
    if (d.isEmpty())
        d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(d);
    return d;
}

Settings::Settings(QObject* parent)
    : QObject(parent), dataDir_(DataDir()), s_(DataDir() + "/settings.ini", QSettings::IniFormat)
{
    g_instance = this;
    QDir().mkpath(nvramDir());
    QDir().mkpath(supportDir());
    QDir().mkpath(artDir());
}

Settings* Settings::instance() { return g_instance; }

void Settings::put(const QString& key, const QVariant& v)
{
    if (s_.value(key) == v)
        return;
    s_.setValue(key, v);
    s_.sync();
    emit changed();
}

QString Settings::romDir() const
{
    if (!romOverride_.isEmpty())
        return romOverride_;
    QString d = s_.value("library/rom_dir").toString();
    if (!d.isEmpty())
        return d;
    // first run: a "roms" folder beside the app, else the folder this project's ROM sets were found in
    const QString beside = QCoreApplication::applicationDirPath() + "/roms";
    if (QDir(beside).exists())
        return beside;
    const QString desktop =
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/UMKL";
    if (QDir(desktop).exists())
        return desktop;
    return beside;
}

QString Settings::playerName() const
{
    QString n = s_.value("online/name").toString();
    return n.isEmpty() ? QStringLiteral("PLAYER") : n;
}

QString Settings::coreDll() const
{
    return QCoreApplication::applicationDirPath() + "/mka_fbneo.dll";
}
