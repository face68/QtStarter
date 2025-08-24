#pragma once
#include <QString>
#include <QMetaType>

namespace App {
	struct AppItem {
		QString name;
		QString path;
		QString workingDir;
		QString args;
		bool    checked;
		bool    uac;
		AppItem() : checked( false ), uac( false ) {
		}
		AppItem( const QString& name, const QString& path, const QString& workingDir, const QString& args, bool checked, bool uac = false )
			: name( name ), path( path ), workingDir( workingDir ), args( args ), checked( checked ), uac( uac ) {
		}
	};
}

Q_DECLARE_METATYPE( App::AppItem )

// Custom role keys
namespace Roles {
	enum: int {
		TypeRole = Qt::UserRole + 1,  // 0=folder, 1=app
		AppDataRole,                  // QVariant::fromValue(AppItem)
	};
}
