#pragma once

#include <QString>

namespace GameSaveSyncError {
enum Type { Network, NotFound, Parsing, Other };
struct Error {
    Type type;
    QString message;
};
} // namespace GameSaveSyncError
