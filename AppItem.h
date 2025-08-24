#pragma once
#include <QString>
#include <QMetaType>

struct AppItem {
	QString name;
	QString path;   // target exe
	QString workingDir;
	QString args;
	bool    checked = false;
	bool	uac;
};

Q_DECLARE_METATYPE( AppItem )

// Custom role keys
namespace Roles {
	enum: int {
		TypeRole = Qt::UserRole + 1,  // 0=folder, 1=app
		AppDataRole,                  // QVariant::fromValue(AppItem)
	};
}
