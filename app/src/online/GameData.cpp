#include "GameData.h"

#include "Style.h"

GameData::GameData()
{
    games_ = {{"mk", "Mortal Kombat", "MK"},
              {"mk2", "Mortal Kombat II", "MK II"},
              {"mk3", "Mortal Kombat 3", "MK 3"},
              {"umk3", "Ultimate Mortal Kombat 3", "UMK 3"}};
}

GameData& GameData::instance()
{
    static GameData d;
    return d;
}

const Game* GameData::game(const QString& id) const
{
    for (const Game& g : games_)
        if (g.id == id)
            return &g;
    return nullptr;
}

namespace style {
QString rankKey(const QString& title)
{
    QString k = title.trimmed().toLower();
    k.replace(QLatin1Char(' '), QLatin1Char('-'));
    return k;
}
}  // namespace style
