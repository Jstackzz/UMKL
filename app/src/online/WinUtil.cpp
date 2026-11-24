#include "WinUtil.h"

#include <QFile>
#include <QSaveFile>

namespace winutil {

bool writeFileAtomic(const QString& path, const QByteArray& data, QString* error, int)
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size() || !f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}

QByteArray readShared(const QString& path, bool* exists)
{
    QFile f(path);
    const bool ok = f.open(QIODevice::ReadOnly);
    if (exists)
        *exists = ok;
    return ok ? f.readAll() : QByteArray();
}

}  // namespace winutil
