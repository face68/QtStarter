#include "MainWindow.h"
#include "LinkResolver.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFileDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QMessageBox>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QApplication>
#include <QPushButton>
#include <QTimer>

#ifdef _WIN32
#include <windows.h>
#endif

enum Col {
	COL_NAME = 0, COL_UAC, COL_ARGS_LABEL, COL_ARGS_VALUE, COL__COUNT
};


static QString startMenuCommon() {
#ifdef _WIN32
	const QString programData = qEnvironmentVariable( "ProgramData" );
	return programData + "/Microsoft/Windows/Start Menu/Programs";
#else
	return QString();
#endif
}

static QString startMenuUser() {
#ifdef _WIN32
	const QString appData = qEnvironmentVariable( "AppData" );
	return appData + "/Microsoft/Windows/Start Menu/Programs";
#else
	return QString();
#endif
}

MainWindow::MainWindow( QWidget* parent ) : QMainWindow( parent ) {

	qRegisterMetaType<AppItem>( "AppItem" );
	buildUi();
	scanStartMenu();
}

void MainWindow::buildUi() {

	auto* central = new QWidget( this );
	setCentralWidget( central );

	auto* v = new QVBoxLayout( central );

	_tree = new QTreeView( central );
	_tree->setModel( &_model );
	_tree->setRootIsDecorated( true );
	_tree->setItemsExpandable( true );
	_tree->setHeaderHidden( false );
	_tree->setUniformRowHeights( true );
	_tree->setEditTriggers( QAbstractItemView::NoEditTriggers );
	_tree->setDragDropMode( QAbstractItemView::NoDragDrop );
	_tree->setSelectionMode( QAbstractItemView::SingleSelection );

	connect( _tree, &QTreeView::doubleClicked, this, &MainWindow::onTreeDoubleClicked );
	connect( &_model, &QStandardItemModel::dataChanged, this, &MainWindow::onModelDataChanged );

	v->addWidget( _tree, 1 );

	auto* h = new QHBoxLayout();
	auto* btnSave = new QPushButton( "Batch speichern", central );
	auto* btnLoad = new QPushButton( "Batch laden", central );
	auto* btnAdd = new QPushButton( "EXE/BAT hinzufuegen", central );
	connect( btnSave, &QPushButton::clicked, this, &MainWindow::onSaveBatch );
	connect( btnLoad, &QPushButton::clicked, this, &MainWindow::onLoadBatch );
	connect( btnAdd, &QPushButton::clicked, this, &MainWindow::onAddExecutable );

	h->addStretch();
	h->addWidget( btnAdd );
	h->addWidget( btnLoad );
	h->addWidget( btnSave );
	v->addLayout( h );

	_selectedList = new QListWidget( central );
	_selectedList->setMaximumHeight( 120 );
	connect( _selectedList, &QListWidget::itemChanged, this, &MainWindow::onSelectedListItemChanged );

	v->addWidget( _selectedList );

	_model.setHorizontalHeaderLabels( { "Name" } );
}

void MainWindow::scanStartMenu() {

	_pathIndex.clear();
	_model.clear();
	_model.setHorizontalHeaderLabels( { "Name" } );

	// 1) Startmenü-Wurzeln
	const QStringList roots = {
		startMenuCommon(), // ProgramData...\Programs
		startMenuUser()    // AppData...\Programs
	};

	// 2) Buckets: key = relativer Ordnerpfad ("" = Root-Apps)
	QMap<QString, QVector<AppItem>> buckets;

	for( const QString& root : roots ) {

		if( root.isEmpty() || !QDir( root ).exists() ) continue;

		QDirIterator it( root, { "*.lnk","*.exe" }, QDir::Files, QDirIterator::Subdirectories );
		while( it.hasNext() ) {
			const QString file = it.next();

			QString target = file;
			if( file.endsWith( ".lnk", Qt::CaseInsensitive ) ) {
				target = resolveShortcutTarget( file );
				if( target.isEmpty() || !target.endsWith( ".exe", Qt::CaseInsensitive ) ) continue;
			}
			else if( !file.endsWith( ".exe", Qt::CaseInsensitive ) ) {
				continue;
			}

			const QString rel = getRelativeFolder( root, file ); // → "" für extern/Root
			QString name = QFileInfo( file ).completeBaseName();
			if( name.contains( "Uninstall", Qt::CaseInsensitive ) || name.contains( "deinstall", Qt::CaseInsensitive ) )
				continue;

			AppItem app{ name, target, QString(), false };
			buckets[ rel ].push_back( app );
			_pathIndex.insert( app.path.toLower(), app );
		}
	}

	// 3) Ordnerkette-CACHE für schnellen Aufbau
	QHash<QString, QStandardItem*> folderNodeByPath; // key: "A/B/C" (leer = Root)
	folderNodeByPath.insert( QString(), _model.invisibleRootItem() );

	auto ensureFolder = [&] ( const QString& relPath ) -> QStandardItem* {

		if( relPath.isEmpty() )
			return _model.invisibleRootItem();

		if( auto it = folderNodeByPath.find( relPath ); it != folderNodeByPath.end() )
			return it.value();

		// baue schrittweise auf
		QStringList parts = relPath.split( QRegularExpression( "[/\\\\]" ), Qt::SkipEmptyParts );
		QString acc;
		QStandardItem* parent = _model.invisibleRootItem();
		for( const QString& part : parts ) {

			acc = acc.isEmpty() ? part : acc + "/" + part;
			if( auto hit = folderNodeByPath.find( acc ); hit != folderNodeByPath.end() ) {

				parent = hit.value();
				continue;
			}

			auto* folder = new QStandardItem( part );
			folder->setEditable( false );
			folder->setData( 0, Roles::TypeRole ); // Folder
			parent->appendRow( folder );
			parent = folder;
			folderNodeByPath.insert( acc, folder );
		}
		return parent;
		};

	// 4) ZUERST Root-Apps (Key = ""), alphabetisch
	{
		auto apps = buckets.value( QString() );
		std::sort( apps.begin(), apps.end(), [] ( const AppItem& a, const AppItem& b ) {
			return a.name.localeAwareCompare( b.name ) < 0;
				   } );
		for( const auto& app : apps ) {

			auto* row = new QStandardItem( app.name );
			row->setEditable( false );
			row->setFlags( row->flags() | Qt::ItemIsUserCheckable );
			row->setData( app.checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole );
			row->setData( 1, Roles::TypeRole );                          // App
			row->setData( QVariant::fromValue( app ), Roles::AppDataRole ); // Payload
			_model.invisibleRootItem()->appendRow( row );
		}
		buckets.remove( QString() );
	}

	// 5) Dann alle übrigen Ordner in QMap-Order (alphabetisch), Apps je Ordner alphabetisch
	for( auto it = buckets.cbegin(); it != buckets.cend(); ++it ) {

		auto apps = it.value();
		std::sort( apps.begin(), apps.end(), [] ( const AppItem& a, const AppItem& b ) {

			return a.name.localeAwareCompare( b.name ) < 0;
				   } );
		QStandardItem* folder = ensureFolder( it.key() );
		for( const auto& app : apps ) {

			auto* row = new QStandardItem( app.name );
			row->setEditable( false );
			row->setFlags( row->flags() | Qt::ItemIsUserCheckable );
			row->setData( app.checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole );
			row->setData( 1, Roles::TypeRole );
			row->setData( QVariant::fromValue( app ), Roles::AppDataRole );
			folder->appendRow( row );
		}
	}

	updateSelectedList();
}

QStandardItem* MainWindow::ensureFolderPath( const QString& relFolder ) {

	QStandardItem* parent = _model.invisibleRootItem();

	const QString clean = relFolder.trimmed();
	if( clean.isEmpty() || clean == "(root)" || clean == "(external)" )
		return parent;

	const QStringList parts = clean.split( QRegularExpression( "[/\\\\]" ), Qt::SkipEmptyParts );

	for( const QString& part : parts ) {
		QStandardItem* match = nullptr;

		// defensiv: child(r,0) kann null sein -> überspringen
		for( int r = 0; r < parent->rowCount(); ++r ) {
			QStandardItem* it = parent->child( r, 0 );
			if( !it ) continue;
			if( it->data( Roles::TypeRole ).toInt() == 0 &&
				it->text().compare( part, Qt::CaseInsensitive ) == 0 ) {
				match = it;
				break;
			}
		}

		if( !match ) {
			match = new QStandardItem( part );
			match->setEditable( false );
			match->setData( 0, Roles::TypeRole );        // Folder
			parent->appendRow( match );                  // garantiert Spalte 0
		}

		parent = match;
	}
	return parent;
}

QString MainWindow::getRelativeFolder( const QString& root, const QString& fullPath ) const {

	// normalisieren
	QString normRoot = QDir::fromNativeSeparators( QDir( root ).absolutePath() );
	QString normFullPath = QDir::fromNativeSeparators( QFileInfo( fullPath ).absoluteFilePath() );
	QString dir = QFileInfo( normFullPath ).absolutePath();

	if( !normRoot.endsWith( '/' ) ) normRoot += '/';

	if( dir.startsWith( normRoot, Qt::CaseInsensitive ) ) {
		QString rel = dir.mid( normRoot.size() );
		rel.remove( QRegularExpression( "^/+" ) );
		rel.remove( QRegularExpression( "/+$" ) );
		return rel; // z.B. "Vendor/App"
	}
	return "";
}

void MainWindow::onTreeDoubleClicked( const QModelIndex& idx ) {

	if( !idx.isValid() )
		return;

	const int type = idx.data( Roles::TypeRole ).toInt();
	if( type == 0 ) {

		// folder -> expand/collapse
		_tree->setExpanded( idx, !_tree->isExpanded( idx ) );
		return;
	}

	// app: toggle checked / list; double-click won't immediately start the app
	AppItem app = idx.data( Roles::AppDataRole ).value<AppItem>();
	app.checked = !app.checked;
	_model.setData( idx, QVariant::fromValue( app ), Roles::AppDataRole );

	_model.dataChanged( idx, idx, { Roles::AppDataRole } );
	toggleAppItem( idx, !app.checked );
}

void MainWindow::onModelDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles ) {

	// Only interested in single-item changes
	if( topLeft != bottomRight )
		return;

	// Only interested in check state changes
	if( !roles.isEmpty() && !roles.contains( Qt::CheckStateRole ) )
		return;

	QStandardItem* item = _model.itemFromIndex( topLeft );
	if( !item )
		return;

	// Only for App items (not folders)
	if( item->data( Roles::TypeRole ).toInt() != 1 )
		return;

	// Get the AppItem
	AppItem app = item->data( Roles::AppDataRole ).value<AppItem>();
	bool checked = item->data( Qt::CheckStateRole ).toInt() == Qt::Checked;

	toggleAppItem( topLeft, checked );
}

void MainWindow::onSelectedListItemChanged( QListWidgetItem* item ) {
	if( !item )
		return;

	// Only act if the checkbox was unchecked (i.e., user wants to remove)
	if( item->checkState() == Qt::Unchecked ) {
		QString path = item->data( Qt::UserRole ).toString();
		// Find the corresponding item in the tree and uncheck it
		std::function<bool( QStandardItem* )> findAndUncheck = [&] ( QStandardItem* it ) -> bool {
			if( !it ) return false;
			if( it->data( Roles::TypeRole ).toInt() == 1 ) {
				AppItem app = it->data( Roles::AppDataRole ).value<AppItem>();
				if( app.path.compare( path, Qt::CaseInsensitive ) == 0 ) {
					app.checked = false;
					it->setData( QVariant::fromValue( app ), Roles::AppDataRole );
					it->setData( Qt::Unchecked, Qt::CheckStateRole );
					// Collapse folder if no other checked
					QStandardItem* parent = it->parent();
					if( parent ) {
						bool anyChecked = false;
						for( int r = 0; r < parent->rowCount(); ++r ) {
							QStandardItem* sibling = parent->child( r, 0 );
							if( sibling && sibling->data( Roles::TypeRole ).toInt() == 1 ) {
								AppItem siblingApp = sibling->data( Roles::AppDataRole ).value<AppItem>();
								if( siblingApp.checked ) {
									anyChecked = true;
									break;
								}
							}
						}
						if( !anyChecked ) {
							QModelIndex parentIdx = _model.indexFromItem( parent );
							_tree->setExpanded( parentIdx, false );
						}
					}
					return true;
				}
			}
			for( int r = 0; r < it->rowCount(); ++r )
				if( findAndUncheck( it->child( r ) ) )
					return true;
			return false;
			};
		QStandardItem* root = _model.invisibleRootItem();
		for( int r = 0; r < root->rowCount(); ++r )
			if( findAndUncheck( root->child( r ) ) )
				break;
	}
}

void MainWindow::toggleAppItem( const QModelIndex& idx, bool checked ) {


	if( !idx.isValid() )
		return;

	AppItem app = idx.data( Roles::AppDataRole ).value<AppItem>();
	app.checked = checked;
	_model.setData( idx, QVariant::fromValue( app ), Roles::AppDataRole );
	// emit dataChanged so delegate updates checkbox
	_model.dataChanged( idx, idx, { Roles::AppDataRole } );
	updateSelectedList();
	//emit appItemCheckToggled( app, checked );
}

void MainWindow::updateSelectedList() {

	_selectedList->clear();
	// walk all items
	std::function<void( QStandardItem* )> walk = [&] ( QStandardItem* it ) {

		if( !it ) return;
		const int t = it->data( Roles::TypeRole ).toInt();
		if( t == 1 ) {

			const AppItem app = it->data( Roles::AppDataRole ).value<AppItem>();
			if( app.checked ) {

				auto* item = new QListWidgetItem( app.name + " — " + app.path + ( app.args.isEmpty() ? QString() : ( " " + app.args ) ) );
				item->setFlags( item->flags() | Qt::ItemIsUserCheckable );
				item->setCheckState( Qt::Checked );
				// Store the app path for lookup
				item->setData( Qt::UserRole, app.path );
				_selectedList->addItem( item );
			}
		}
		for( int r = 0; r < it->rowCount(); ++r )
			walk( it->child( r ) );
		};

	QStandardItem* root = _model.invisibleRootItem();
	for( int r = 0; r < root->rowCount(); ++r )
		walk( root->child( r ) );
}

void MainWindow::onSaveBatch() {
	const QString fileName = QFileDialog::getSaveFileName( this, "Batch speichern", QString(), "Batch (*.bat)" );
	if( fileName.isEmpty() ) return;

	QFile f( fileName );
	if( !f.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) ) {
		QMessageBox::warning( this, "Fehler", "Datei kann nicht geschrieben werden." );
		return;
	}
	QTextStream out( &f );
	out << "@echo off\n";

	// iterate
	std::function<void( QStandardItem* )> walk = [&] ( QStandardItem* it ) {
		if( !it ) return;
		if( it->data( Roles::TypeRole ).toInt() == 1 ) {
			const AppItem app = it->data( Roles::AppDataRole ).value<AppItem>();
			if( app.checked ) {
				const QString safeName = app.name.trimmed().replace( '|', ' ' );
				out << "REM Name=" << safeName << " | Args=" << app.args << "\n";
				out << "start \"\" \"" << app.path << "\" " << app.args << "\n";
			}
		}
		for( int r = 0; r < it->rowCount(); ++r ) walk( it->child( r ) );
		};

	QStandardItem* root = _model.invisibleRootItem();
	for( int r = 0; r < root->rowCount(); ++r ) walk( root->child( r ) );
}

void MainWindow::onLoadBatch() {

	const QString fileName = QFileDialog::getOpenFileName( this, "Batch laden", QString(), "Batch (*.bat)" );
	if( fileName.isEmpty() ) return;

	QFile f( fileName );
	if( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) return;
	QTextStream in( &f );

	struct Entry {
		QString name, path, args;
	};
	QVector<Entry> entries;
	QString lastRem;

	while( !in.atEnd() ) {

		const QString l = in.readLine().trimmed();
		if( l.startsWith( "REM ", Qt::CaseInsensitive ) ) {

			lastRem = l; continue;
		}
		if( l.startsWith( "start ", Qt::CaseInsensitive ) ) {

			entries.push_back( { parseRemName( lastRem ), extractQuotedPath( l ), extractArgsAfterQuotedPath( l ) } );
			lastRem.clear();
		}
	}

	// uncheck all
	std::function<void( QStandardItem* )> uncheck = [&] ( QStandardItem* it ) {
		if( !it ) return;
		if( it->data( Roles::TypeRole ).toInt() == 1 ) {
			AppItem app = it->data( Roles::AppDataRole ).value<AppItem>();
			app.checked = false;
			app.args.clear();
			it->setData( QVariant::fromValue( app ), Roles::AppDataRole );
			it->setData( Qt::CheckState::Unchecked, Qt::CheckStateRole );

		}
		for( int r = 0; r < it->rowCount(); ++r ) uncheck( it->child( r ) );
		};
	QStandardItem* root = _model.invisibleRootItem();
	for( int r = 0; r < root->rowCount(); ++r ) uncheck( root->child( r ) );

	// mark or create under (external)
	auto ensureExternal = [&] () { return ensureFolderPath( "" ); };

	for( const auto& e : entries ) {

		const QString key = e.path.toLower();
		bool found = false;
		// scan find (linear; dataset size is small)
		std::function<void( QStandardItem* )> find = [&] ( QStandardItem* it ) {

			if( !it || found ) 
				return;

			if( it->data( Roles::TypeRole ).toInt() == 1 ) {

				AppItem app = it->data( Roles::AppDataRole ).value<AppItem>();

				if( app.path.compare( e.path, Qt::CaseInsensitive ) == 0 ) {

					app.args = e.args;
					app.checked = true;
					it->setData( QVariant::fromValue( app ), Roles::AppDataRole );
					it->setData( Qt::CheckState::Checked, Qt::CheckStateRole );
					// expand parents
					QModelIndex idx = _model.indexFromItem( it );
					while( idx.isValid() ) {

						_tree->setExpanded( idx.parent(), true );
						idx = idx.parent();
					}

					found = true;
					return;
				}
			}
			for( int r = 0; r < it->rowCount(); ++r ) find( it->child( r ) );
			};
		for( int r = 0; r < root->rowCount() && !found; ++r ) find( root->child( r ) );

		if( !found ) {

			QStandardItem* ext = ensureExternal();
			AppItem app{ e.name.isEmpty() ? QFileInfo( e.path ).completeBaseName() : e.name, e.path, e.args, true };
			QStandardItem* row = new QStandardItem( app.name );
			row->setData( 1, Roles::TypeRole );
			row->setData( QVariant::fromValue( app ), Roles::AppDataRole );
			row->setData( Qt::Checked, Qt::CheckStateRole );
			ext->appendRow( row );
			_pathIndex.insert( app.path.toLower(), app );
		}
	}

	updateSelectedList();
}

void MainWindow::onAddExecutable() {

	const QString file = QFileDialog::getOpenFileName( this, "Programm auswaehlen", QString(), "Programme (*.exe *.bat)" );
	if( file.isEmpty() ) return;

	// if exists in index -> check it & expand
	bool found = false;
	std::function<void( QStandardItem* )> find = [&] ( QStandardItem* it ) {
		if( !it || found ) return;
		if( it->data( Roles::TypeRole ).toInt() == 1 ) {
			AppItem app = it->data( Roles::AppDataRole ).value<AppItem>();
			if( app.path.compare( file, Qt::CaseInsensitive ) == 0 ) {
				app.checked = true; it->setData( QVariant::fromValue( app ), Roles::AppDataRole );
				QModelIndex idx = _model.indexFromItem( it );
				while( idx.isValid() ) {
					_tree->setExpanded( idx.parent(), true ); idx = idx.parent();
				}
				found = true; return;
			}
		}
		for( int r = 0; r < it->rowCount(); ++r ) find( it->child( r ) );
		};

	QStandardItem* root = _model.invisibleRootItem();
	for( int r = 0; r < root->rowCount() && !found; ++r )
		find( root->child( r ) );

	if( !found ) {

		QStandardItem* ext = ensureFolderPath( "(external)" );
		AppItem app{ QFileInfo( file ).completeBaseName(), file, QString(), true };
		QStandardItem* row = new QStandardItem( app.name );
		row->setData( 1, Roles::TypeRole );
		row->setData( QVariant::fromValue( app ), Roles::AppDataRole );
		ext->appendRow( row );
		_pathIndex.insert( app.path.toLower(), app );
	}

	updateSelectedList();
}

QString MainWindow::extractQuotedPath( const QString& line ) {
	int first = line.indexOf( '"' ); if( first < 0 ) return {};
	int second = line.indexOf( '"', first + 1 ); if( second < 0 ) return {};
	int third = line.indexOf( '"', second + 1 ); if( third < 0 ) return {};
	int fourth = line.indexOf( '"', third + 1 ); if( fourth < 0 ) return {};
	return line.mid( third + 1, fourth - third - 1 );
}

QString MainWindow::extractArgsAfterQuotedPath( const QString& line ) {
	int count = 0, fourth = -1;
	for( int i = 0; i < line.size(); ++i ) {
		if( line[ i ] == '"' ) {
			++count; if( count == 4 ) {
				fourth = i; break;
			}
		}
	}
	return fourth < 0 ? QString() : line.mid( fourth + 1 ).trimmed();
}

QString MainWindow::parseRemName( const QString& remLine ) {
	if( !remLine.startsWith( "REM ", Qt::CaseInsensitive ) ) return {};
	const QString body = remLine.mid( 4 );
	const QStringList parts = body.split( '|', Qt::SkipEmptyParts );
	for( const QString& p : parts ) {
		const int eq = p.indexOf( '=' );
		if( eq > 0 ) {
			const QString k = p.left( eq ).trimmed();
			if( k.compare( "Name", Qt::CaseInsensitive ) == 0 ) return p.mid( eq + 1 ).trimmed();
		}
	}
	return {};
}