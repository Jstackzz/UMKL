#pragma once
// The four games and whether the player's ROM folder holds them: each set's ROMs, as the emulation core lists them,
// against the CRCs inside the player's zips. Nothing from the games ships with the app.
#include <QObject>
#include <QString>
#include <QVariantList>

class Settings;

class Library : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList games READ games NOTIFY changed)
    Q_PROPERTY(int readyCount READ readyCount NOTIFY changed)
    Q_PROPERTY(QString romDir READ romDir NOTIFY changed)

public:
    explicit Library(Settings* settings, QObject* parent = nullptr);

    // looks at the ROM folder again (after the player changed it or added zips)
    Q_INVOKABLE void refresh();
    // the picture a game's card shows: the best moment of its attract mode seen so far, as a file URL ("" before)
    Q_INVOKABLE QString artUrl(const QString& set) const;
    Q_INVOKABLE QVariantMap game(const QString& id) const;

    QVariantList games() const { return games_; }
    int readyCount() const;
    QString romDir() const;

signals:
    void changed();

private:
    Settings* settings_;
    QVariantList games_;
};
