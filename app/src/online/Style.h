#pragma once
// The part of MK3D's launcher Style its online client uses.
#include <QString>

namespace style {
// a ranked title as the badges name it: "Chosen One" -> "chosen-one"
QString rankKey(const QString& title);
}  // namespace style
