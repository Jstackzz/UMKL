#pragma once
// The player's settings (an INI file in %APPDATA%\MK Arcade Online) and the folders the app keeps its data in.
#include <QObject>
#include <QSettings>
#include <QString>

class Settings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString romDir READ romDir WRITE setRomDir NOTIFY changed)
    Q_PROPERTY(bool fullscreen READ fullscreen WRITE setFullscreen NOTIFY changed)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY changed)
    Q_PROPERTY(QString scaleMode READ scaleMode WRITE setScaleMode NOTIFY changed)  // fit, stretch, pixel
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY changed)
    Q_PROPERTY(int menuVolume READ menuVolume WRITE setMenuVolume NOTIFY changed)  // the attract mode behind menus
    Q_PROPERTY(QString playerName READ playerName WRITE setPlayerName NOTIFY changed)
    Q_PROPERTY(QString server READ server WRITE setServer NOTIFY changed)
    Q_PROPERTY(int inputDelay READ inputDelay WRITE setInputDelay NOTIFY changed)
    Q_PROPERTY(int netPort READ netPort WRITE setNetPort NOTIFY changed)
    Q_PROPERTY(QString lastPeer READ lastPeer WRITE setLastPeer NOTIFY changed)
    Q_PROPERTY(QString region READ region WRITE setRegion NOTIFY changed)
    Q_PROPERTY(int maxPing READ maxPing WRITE setMaxPing NOTIFY changed)
    Q_PROPERTY(bool searchAgain READ searchAgain WRITE setSearchAgain NOTIFY changed)
    Q_PROPERTY(bool hideIp READ hideIp WRITE setHideIp NOTIFY changed)
    Q_PROPERTY(QString dataDir READ dataDir CONSTANT)

public:
    explicit Settings(QObject* parent = nullptr);
    static Settings* instance();

    QString romDir() const;
    void setRomDir(const QString& v) { put("library/rom_dir", v); }
    // tests: a ROM folder for this run only, the settings file untouched
    void overrideRomDir(const QString& v) { romOverride_ = v; }
    bool fullscreen() const { return !windowedOverride_ && s_.value("display/fullscreen", true).toBool(); }
    void overrideFullscreen(bool on) { windowedOverride_ = !on; }
    void setFullscreen(bool v) { put("display/fullscreen", v); }
    bool smooth() const { return s_.value("display/smooth", false).toBool(); }
    void setSmooth(bool v) { put("display/smooth", v); }
    QString scaleMode() const { return s_.value("display/scale", "fit").toString(); }
    void setScaleMode(const QString& v) { put("display/scale", v); }
    int volume() const { return s_.value("sound/volume", 80).toInt(); }
    void setVolume(int v) { put("sound/volume", qBound(0, v, 100)); }
    int menuVolume() const { return s_.value("sound/menu_volume", 35).toInt(); }
    void setMenuVolume(int v) { put("sound/menu_volume", qBound(0, v, 100)); }
    QString playerName() const;
    void setPlayerName(const QString& v) { put("online/name", v.left(16)); }
    QString savedName() const { return s_.value("online/name").toString(); }  // empty before a first sign-in
    // the MK3D server on Oracle Cloud, shared with MK3D (the server keeps the two products apart); its certificate
    // is pinned by fingerprint
    QString server() const
    {
        return !serverOverride_.isEmpty() ? serverOverride_
                                          : s_.value("online/server", "129.80.13.245:7777").toString();
    }
    void setServer(const QString& v) { put("online/server", v); }
    QString fingerprint() const
    {
        return !fingerprintOverride_.isEmpty()
                   ? fingerprintOverride_
                   : s_.value("online/fingerprint",
                              "C9:D3:E6:40:B9:8D:4E:1C:B6:5C:9A:2A:67:3F:D4:31:51:34:73:69:A2:CC:3E:0E:46:68:B9:93:69:00:D4:C1")
                         .toString();
    }
    // tests: another server (a local one) and game port for this run only, the settings file untouched
    void overrideServer(const QString& address, const QString& fingerprint)
    {
        serverOverride_ = address;
        fingerprintOverride_ = fingerprint;
    }
    void overrideNetPort(int port) { portOverride_ = port; }
    QString region() const { return s_.value("online/region", "na").toString(); }
    void setRegion(const QString& v) { put("online/region", v); }
    int maxPing() const { return s_.value("online/max_ping", 0).toInt(); }  // 0: any
    void setMaxPing(int v) { put("online/max_ping", qMax(0, v)); }
    bool searchAgain() const { return s_.value("online/search_again", true).toBool(); }
    void setSearchAgain(bool v) { put("online/search_again", v); }
    bool hideIp() const { return s_.value("online/hide_ip", false).toBool(); }
    void setHideIp(bool v) { put("online/hide_ip", v); }
    // "remember me": the server's login token (30 days from its last use), empty when none
    QString token() const { return s_.value("online/token", "").toString(); }
    void setToken(const QString& v) { put("online/token", v); }
    // tests: two copies on one PC reach each other over loopback only
    bool loopbackOnly() const { return loopback_; }
    void overrideLoopback(bool on) { loopback_ = on; }
    int inputDelay() const { return s_.value("online/delay", 2).toInt(); }
    void setInputDelay(int v) { put("online/delay", qBound(0, v, 6)); }
    int netPort() const { return portOverride_ > 0 ? portOverride_ : s_.value("online/port", 7101).toInt(); }
    void setNetPort(int v) { put("online/port", qBound(1024, v, 65535)); }
    QString lastPeer() const { return s_.value("online/last_peer", "").toString(); }
    void setLastPeer(const QString& v) { put("online/last_peer", v); }

    QString dataDir() const { return dataDir_; }
    QString nvramDir() const { return dataDir_ + "/nvram"; }
    QString supportDir() const { return dataDir_ + "/support"; }
    QString artDir() const { return dataDir_ + "/art"; }
    QString coreDll() const;

signals:
    void changed();

private:
    void put(const QString& key, const QVariant& v);
    QString dataDir_;
    QString romOverride_;
    bool windowedOverride_ = false;
    bool loopback_ = false;
    QString serverOverride_, fingerprintOverride_;
    int portOverride_ = 0;
    mutable QSettings s_;
};
