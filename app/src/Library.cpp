#include "Library.h"

#include "EmuCore.h"
#include "Settings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QUrl>
#include <QtEndian>

namespace {

struct GameDef {
    const char* id;
    const char* set;
    const char* title;
    const char* tag;  // the card's short title
    int year;
    const char* board;
    const char* blurb;
    bool run;  // has a RUN button
};

const GameDef kGames[] = {
    {"mk", "mk", "Mortal Kombat", "MK", 1992, "Midway T-Unit",
     "The tournament begins. Seven fighters, Goro and Shang Tsung.", false},
    {"mk2", "mk2", "Mortal Kombat II", "MK II", 1993, "Midway T-Unit",
     "Outworld. Twelve fighters, Friendships, Babalities and Kintaro.", false},
    {"mk3", "mk3", "Mortal Kombat 3", "MK 3", 1995, "Midway Wolf Unit",
     "Earth invaded. The RUN button, combos and Animalities.", true},
    {"umk3", "umk3", "Ultimate Mortal Kombat 3", "UMK 3", 1995, "Midway Wolf Unit",
     "The tournament favourite. Every ninja, Endurance and Mileena.", true},
};

// the CRC32 of every member of a zip, from its central directory (no member is unpacked)
QSet<quint32> ZipCrcs(const QString& path)
{
    QSet<quint32> crcs;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return crcs;
    const qint64 size = f.size();
    const qint64 tail = qMin<qint64>(size, 65536 + 22);
    f.seek(size - tail);
    const QByteArray t = f.read(tail);
    int eocd = -1;
    for (int i = t.size() - 22; i >= 0; i--)
        if (qFromLittleEndian<quint32>(t.constData() + i) == 0x06054b50) {
            eocd = i;
            break;
        }
    if (eocd < 0)
        return crcs;
    const quint32 cdSize = qFromLittleEndian<quint32>(t.constData() + eocd + 12);
    const quint32 cdOffset = qFromLittleEndian<quint32>(t.constData() + eocd + 16);
    f.seek(cdOffset);
    const QByteArray cd = f.read(cdSize);
    int p = 0;
    while (p + 46 <= cd.size() && qFromLittleEndian<quint32>(cd.constData() + p) == 0x02014b50) {
        crcs.insert(qFromLittleEndian<quint32>(cd.constData() + p + 16));
        const int nameLen = qFromLittleEndian<quint16>(cd.constData() + p + 28);
        const int extraLen = qFromLittleEndian<quint16>(cd.constData() + p + 30);
        const int commentLen = qFromLittleEndian<quint16>(cd.constData() + p + 32);
        p += 46 + nameLen + extraLen + commentLen;
    }
    return crcs;
}

}  // namespace

Library::Library(Settings* settings, QObject* parent) : QObject(parent), settings_(settings)
{
    connect(settings_, &Settings::changed, this, [this] {
        static QString last;
        if (settings_->romDir() != last) {
            last = settings_->romDir();
            refresh();
        }
    });
    refresh();
}

QString Library::romDir() const { return QDir::toNativeSeparators(settings_->romDir()); }

void Library::refresh()
{
    // a private copy of the core reads the ROM tables, so a game running in the main copy is not touched
    const QString cache = settings_->dataDir() + "/cache";
    QDir().mkpath(cache);
    const QString tables = cache + "/mka_tables.dll";
    QFile::remove(tables);
    QFile::copy(settings_->coreDll(), tables);
    EmuCore core;
    QString err;
    const bool haveCore = core.load(tables, &err);

    games_.clear();
    const QString dir = settings_->romDir();
    for (const GameDef& g : kGames) {
        QVariantMap m;
        m["id"] = g.id;
        m["set"] = g.set;
        m["title"] = g.title;
        m["tag"] = g.tag;
        m["year"] = g.year;
        m["board"] = g.board;
        m["blurb"] = g.blurb;
        m["run"] = g.run;
        const QString zip = dir + "/" + g.set + ".zip";
        m["zip"] = QDir::toNativeSeparators(zip);
        int total = 0, missing = 0;
        if (haveCore) {
            const int idx = core.driverIndex(g.set);
            const QString parent = core.driverText(idx, 2);
            QSet<quint32> crcs = ZipCrcs(zip);
            if (!parent.isEmpty())
                crcs += ZipCrcs(dir + "/" + parent + ".zip");
            EmuCore::Rom r;
            for (int i = 0; core.driverRom(idx, i, &r); i++) {
                if (r.crc == 0 || r.length == 0 || (r.type & ((1u << 27) | (1u << 28))))  // optional, no dump
                    continue;
                total++;
                if (!crcs.contains(r.crc))
                    missing++;
            }
        }
        const bool present = QFileInfo::exists(zip);
        const bool ready = haveCore && present && total > 0 && missing == 0;
        m["ready"] = ready;
        m["roms"] = total;
        m["missing"] = missing;
        m["status"] = !haveCore ? QStringLiteral("The emulation core is missing")
                      : !present ? QStringLiteral("%1.zip is not in your ROM folder").arg(g.set)
                      : missing  ? QStringLiteral("%1 of %2 ROMs missing from %3.zip").arg(missing).arg(total).arg(g.set)
                                 : QStringLiteral("Ready");
        games_ << m;
    }
    core.unload();
    QFile::remove(tables);
    emit changed();
}

int Library::readyCount() const
{
    int n = 0;
    for (const QVariant& v : games_)
        n += v.toMap().value("ready").toBool() ? 1 : 0;
    return n;
}

QString Library::artUrl(const QString& set) const
{
    const QString p = settings_->artDir() + "/" + set + ".png";
    return QFileInfo::exists(p) ? QUrl::fromLocalFile(p).toString() : QString();
}

QVariantMap Library::game(const QString& id) const
{
    for (const QVariant& v : games_)
        if (v.toMap().value("id").toString() == id)
            return v.toMap();
    return {};
}
