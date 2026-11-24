#pragma once
// The part of MK3D's launcher WinUtil its online client uses. Freezing and thawing processes is a no-op here: the
// game runs inside this app, whose own process must never be frozen.
#include <QByteArray>
#include <QList>
#include <QString>

namespace winutil {
bool writeFileAtomic(const QString& path, const QByteArray& data, QString* error = nullptr, int retryMs = 1000);
QByteArray readShared(const QString& path, bool* exists = nullptr);
inline bool freezeProcess(qint64, QList<quintptr>*) { return true; }
inline bool thawProcess(qint64, const QList<quintptr>&) { return true; }
inline void backgroundMode(bool) {}
}  // namespace winutil
