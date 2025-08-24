#pragma once
#include <QString>

struct ShortcutInfo {
	QString path;        // tatsaechliche EXE
	QString arguments;   // aus der LNK
	QString workingDir;  // aus der LNK
	bool    isMsi = false;
};

// Windows-only: resolve .lnk; empty path if fails
ShortcutInfo resolveShortcut( const QString& lnkPath );
