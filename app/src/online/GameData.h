#pragma once
// The games, as MK3D's MatchController names them in its log.
#include <QList>
#include <QString>

struct Game {
    QString id;
    QString title;
    QString shortName;
};

class GameData {
public:
    static GameData& instance();
    const QList<Game>& games() const { return games_; }
    const Game* game(const QString& id) const;

private:
    GameData();
    QList<Game> games_;
};
