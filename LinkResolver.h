#pragma once
#include <QString>

// Windows-only: resolve .lnk target; returns empty if fails
QString resolveShortcutTarget( const QString& lnkPath );