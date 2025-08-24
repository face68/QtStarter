#include "MainWindow.h"
#include "LinkResolver.h"
#include "AppListDelegate.h"
#include "AppFilterProxyModel.h"

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
#include <QHeaderView>
#include <QScrollBar>
#include <QSplitter>

#include <windows.h>

auto dump = [&] ( const QPalette& pal, QPalette::ColorGroup g ) {
	qDebug() << "Base" << pal.color( g, QPalette::Base );
	qDebug() << "Alt " << pal.color( g, QPalette::AlternateBase );
	qDebug() << "Win " << pal.color( g, QPalette::Window );
	};

static QString startMenuCommon() {
	const QString programData = qEnvironmentVariable( "ProgramData" );
	return programData + "/Microsoft/Windows/Start Menu/Programs";
}

static QString startMenuUser() {
	const QString appData = qEnvironmentVariable( "AppData" );
	return appData + "/Microsoft/Windows/Start Menu/Programs";
}

MainWindow::MainWindow( QWidget* parent ) : QMainWindow( parent ), _buildingSelected( false ) {

	qRegisterMetaType<AppItem>( "AppItem" );
	buildUi();
	scanStartMenu();
	setStyleSheets();
	QTimer::singleShot( 0, this, [this] () { applyListCols(); } );
}

void MainWindow::buildUi() {

	auto* central = new QWidget( this );
	setCentralWidget( central );

	auto* root = new QVBoxLayout( central );
	root->setContentsMargins( 6, 6, 6, 6 );
	root->setSpacing( 6 );

	// Topbar: Suche (links) + Buttons (rechts)
	auto* top = new QHBoxLayout();

	_searchEdit = new QLineEdit( central );
	_searchEdit->setPlaceholderText( "Search..." );
	_searchEdit->setClearButtonEnabled( true );
	_searchEdit->setFixedHeight( 28 );
	top->addWidget( _searchEdit, /*stretch*/ 1 );

	auto* btnAdd = new QPushButton( "Add EXE/BAT", central );
	auto* btnLoad = new QPushButton( "Batch load", central );
	auto* btnSave = new QPushButton( "Batch save", central );
	btnAdd->setFixedHeight( 28 );
	btnLoad->setFixedHeight( 28 );
	btnSave->setFixedHeight( 28 );
	
	top->addWidget( btnAdd );
	top->addWidget( btnLoad );
	top->addWidget( btnSave );

	root->addLayout( top );

	// Modelle / Proxy
	_proxy = new AppFilterProxyModel( this );
	_proxy->setSourceModel( &_model );
	_proxy->setFilterCaseSensitivity( Qt::CaseInsensitive );
	_proxy->setFilterKeyColumn( COL_NAME );

	// Tree (oben)
	_tree = new QTreeView( central );
	_tree->setModel( _proxy );
	_tree->setHeaderHidden( false );
	_tree->setExpandsOnDoubleClick( false );
	_tree->setSelectionMode( QAbstractItemView::NoSelection );
	_tree->setRootIsDecorated( true );
	_tree->setItemsExpandable( true );
	_tree->setAlternatingRowColors( true );
	_tree->setUniformRowHeights( true );
	_tree->setItemDelegateForColumn( COL_UAC, new NoHoverDelegate( 28, _tree ) );
	_tree->setItemDelegateForColumn( COL_ARGS_VALUE, new NoHoverDelegate( 28, _tree ) );

	// Auswahlliste (unten)
	_selectedListModel = new QStandardItemModel( this );
	_selectedListView = new QTableView( central );
	_selectedListView->setItemDelegate( new AppListDelegate( this ) );
	_selectedListView->setModel( _selectedListModel );
	_selectedListView->setSelectionBehavior( QAbstractItemView::SelectRows );
	_selectedListView->setSelectionMode( QAbstractItemView::SingleSelection );
	_selectedListView->setShowGrid( false );
	_selectedListView->setAlternatingRowColors( true );
	_selectedListView->horizontalHeader()->setStretchLastSection( true );
	_selectedListView->horizontalHeader()->setDefaultAlignment( Qt::AlignLeft | Qt::AlignVCenter );
	_selectedListView->verticalHeader()->setVisible( false );
	_selectedListView->setMinimumHeight( 100 ); // statt setMaximumHeight

	// Splitter (Tree oben, Liste unten)
	auto* split = new QSplitter( Qt::Vertical, central );
	split->setChildrenCollapsible( false );
	split->setOpaqueResize( true );
	split->addWidget( _tree );
	split->addWidget( _selectedListView );
	split->setStretchFactor( 0, 3 );
	split->setStretchFactor( 1, 1 );
	root->addWidget( split, /*stretch*/ 1 );

	// Spalten / Labels
	_model.setColumnCount( COL__COUNT );
	_model.setHorizontalHeaderLabels( { "Name", "UAC", "Args" } );

	// Signals
	connect( btnSave, &QPushButton::clicked, this, &MainWindow::onSaveBatch );
	connect( btnLoad, &QPushButton::clicked, this, &MainWindow::onLoadBatch );
	connect( btnAdd, &QPushButton::clicked, this, &MainWindow::onAddExecutable );

	connect( _tree, &QTreeView::doubleClicked, this, &MainWindow::onTreeDoubleClicked );
	connect( &_model, &QStandardItemModel::dataChanged, this, &MainWindow::onModelDataChanged );
	connect( _selectedListView, &QTableView::doubleClicked, this, &MainWindow::onListDoubleClicked );
	connect( _selectedListModel, &QStandardItemModel::itemChanged, this, &MainWindow::onSelectedListItemChanged );
	connect( _searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged );

	// Startgrößen + Spaltenbreiten erst nach Layout
	QTimer::singleShot( 0, this, [this, split] () {

		const QList<int> s = split->sizes();
		if( s.size() == 2 ) {

			const int total = s[ 0 ] + s[ 1 ];
			split->setSizes( { int( total * 0.70 ), int( total * 0.30 ) } );
		}

		applyListCols();
						} );
}

void MainWindow::setStyleSheets() {

	const QString headerQss =
		"QHeaderView::section {"
		"  background: palette(midlight);"
		"  color: palette(window-text);"
		"  font-weight: normal;"
		"}";

	const QString rowQss =
		"QTreeView::item { height:28px; padding:6px 8px; }"
		"QTableView::item { height:28px; padding:6px 8px; }";

	const QString buttonQss = "QPushButton { padding: 0 12px; }";

	if( _tree ) {

		_tree->header()->setStyleSheet( headerQss );
		_tree->setStyleSheet( rowQss );
	}

	if( _selectedListView ) {

		_selectedListView->horizontalHeader()->setStyleSheet( headerQss );
		_selectedListView->setStyleSheet( rowQss );
	}

	this->setStyleSheet( this->styleSheet() + buttonQss );
}

void MainWindow::changeEvent( QEvent* e ) {

	if( e->type() == QEvent::ApplicationPaletteChange
		|| e->type() == QEvent::ThemeChange ) {

		if( _stylingNow ) {
			return;
		}

		_stylingNow = true;
		setStyleSheets();
		_stylingNow = false;
	}

	QMainWindow::changeEvent( e );
}

void MainWindow::applyListCols() {
	// --- Liste ---
	auto* lhdr = _selectedListView->horizontalHeader();

	const int lname = int( lhdr->viewport()->width() * 0.5 );     // 50%
	const int luac = lhdr->fontMetrics().horizontalAdvance( "UAC" ); // fix: nur Textbreite

	lhdr->resizeSection( 0, lname );
	lhdr->resizeSection( 1, luac );

	// Rest exakt ohne Überstand:
	const int lrest = qMax( 0, lhdr->viewport()->width() - lhdr->sectionSize( 0 ) - lhdr->sectionSize( 1 ) );
	lhdr->resizeSection( 2, lrest );

	// --- Tree ---
	auto* thdr = _tree->header();

	const int tname = int( thdr->viewport()->width() * 0.5 );
	const int tuac = thdr->fontMetrics().horizontalAdvance( "UAC" );

	thdr->resizeSection( 0, tname );
	thdr->resizeSection( 1, tuac );

	const int trest = qMax( 0, thdr->viewport()->width() - thdr->sectionSize( 0 ) - thdr->sectionSize( 1 ) );
	thdr->resizeSection( 2, trest );
}


void MainWindow::resizeEvent( QResizeEvent* e ) {

	QMainWindow::resizeEvent( e );
	applyListCols();
}

void MainWindow::scanStartMenu() {

	_pathIndex.clear();
	_model.clear();
	_model.setHorizontalHeaderLabels( { "Name", "UAC", "Args" } );

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

			QString workdir;
			QString target = file;
			if( file.endsWith( ".lnk", Qt::CaseInsensitive ) ) {
				const auto s = resolveShortcut( file );
				target = s.path;
				workdir = s.workingDir;
				// args/workingDir kannst du hier direkt ins AppItem übernehmen, wenn gewünscht:
				// app.args = s.arguments;
				if( target.isEmpty() || !target.endsWith( ".exe", Qt::CaseInsensitive ) ) {
					continue;
				}
			}
			else if( !file.endsWith( ".exe", Qt::CaseInsensitive ) ) {
				continue;
			}

			const QString rel = getRelativeFolder( root, file ); // → "" für extern/Root
			QString name = QFileInfo( file ).completeBaseName();
			if( name.contains( "Uninstall", Qt::CaseInsensitive ) || name.contains( "deinstall", Qt::CaseInsensitive ) )
				continue;

			AppItem app{ name, target, workdir, QString(), false };
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
			folder->setData( 0, Roles::TypeRole );
			//folder->setFlags( folder->flags() & ~Qt::ItemIsSelectable );

			auto* uacDummy = new QStandardItem();
			auto* argsDummy = new QStandardItem();

			// nur Editierbarkeit entfernen (weiter selektierbar/aktiv)
			uacDummy->setFlags( Qt::NoItemFlags );
			uacDummy->setFlags( ( Qt::ItemIsEnabled )
								& ~( Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled ) );

			argsDummy->setFlags( Qt::NoItemFlags );
			argsDummy->setFlags( ( Qt::ItemIsEnabled )
								& ~( Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled ) );

			parent->appendRow( { folder, uacDummy, argsDummy } );
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

			auto* nameIt = new QStandardItem( app.name );
			nameIt->setEditable( false );
			nameIt->setFlags( nameIt->flags() | Qt::ItemIsUserCheckable );
			nameIt->setData( app.checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole );
			nameIt->setData( 1, Roles::TypeRole );
			nameIt->setData( QVariant::fromValue( app ), Roles::AppDataRole );
			//nameIt->setFlags( nameIt->flags() & ~Qt::ItemIsSelectable );

			auto* uacIt = new QStandardItem();
			uacIt->setEditable( false );
			uacIt->setFlags( uacIt->flags() | Qt::ItemIsUserCheckable );
			uacIt->setData( Qt::Unchecked, Qt::CheckStateRole );
			//uacIt->setFlags( uacIt->flags() & ~Qt::ItemIsSelectable );

			auto* argsIt = new QStandardItem( app.args );
			argsIt->setEditable( true );
			//argsIt->setFlags( argsIt->flags() & ~Qt::ItemIsSelectable );

			_model.invisibleRootItem()->appendRow( { nameIt, uacIt, argsIt } );
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

			auto* nameIt = new QStandardItem( app.name );
			nameIt->setEditable( false );
			nameIt->setFlags( nameIt->flags() | Qt::ItemIsUserCheckable );
			nameIt->setData( app.checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole );
			nameIt->setData( 1, Roles::TypeRole );
			nameIt->setData( QVariant::fromValue( app ), Roles::AppDataRole );
			//nameIt->setFlags( nameIt->flags() & ~Qt::ItemIsSelectable );

			auto* uacIt = new QStandardItem();
			uacIt->setEditable( false );
			uacIt->setFlags( uacIt->flags() | Qt::ItemIsUserCheckable );
			uacIt->setData( Qt::Unchecked, Qt::CheckStateRole );
			//uacIt->setFlags( uacIt->flags() & ~Qt::ItemIsSelectable );

			auto* argsIt = new QStandardItem( app.args );
			argsIt->setEditable( true );
			//argsIt->setFlags( argsIt->flags() & ~Qt::ItemIsSelectable );

			folder->appendRow( { nameIt, uacIt, argsIt } );
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
			match->setData( 0, Roles::TypeRole );
			match->setFlags( match->flags() & ~Qt::ItemIsSelectable);

			auto* uacDummy = new QStandardItem();
			auto* argsDummy = new QStandardItem();
			uacDummy->setFlags( ( Qt::ItemIsEnabled )
								 & ~( Qt::ItemIsEditable | Qt::ItemIsSelectable
								 | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled ) );
			argsDummy->setFlags( ( Qt::ItemIsEnabled )
								 & ~( Qt::ItemIsEditable | Qt::ItemIsSelectable
								 | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled ) );

			parent->appendRow( { match, uacDummy, argsDummy } );
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

void MainWindow::onSearchTextChanged( const QString& text ) {

	QRegularExpression rx( QRegularExpression::escape( text ), QRegularExpression::PatternOption::CaseInsensitiveOption );
	_proxy->setFilterRegularExpression( rx );

	if( !text.isEmpty() ) {
		_tree->expandAll();
	}
	else {
		_tree->collapseAll();
	}
}

void MainWindow::onTreeDoubleClicked( const QModelIndex& idx ) {

	if( !idx.isValid() ) {
		return;
	}

	QModelIndex srcIdx = _proxy->mapToSource( idx );
	if( !srcIdx.isValid() ) {
		return;
	}

	const QModelIndex nameIdx = srcIdx.sibling( srcIdx.row(), COL_NAME );

	// Für View-Operationen zurück in Proxy mappen:
	const QModelIndex proxyNameIdx = _proxy->mapFromSource( nameIdx );

	if( nameIdx.data( Roles::TypeRole ).toInt() == 0 ) {

		const bool ex = _tree->isExpanded( proxyNameIdx );
		_tree->setExpanded( proxyNameIdx, !ex );
		return;
	}

	auto type = srcIdx.data( Roles::TypeRole );
	if( type == 1 ) {
		auto cs = static_cast< Qt::CheckState >( srcIdx.data( Qt::CheckStateRole ).toInt() );
		_model.setData( srcIdx, ( cs == Qt::Checked ? Qt::Unchecked : Qt::Checked ), Qt::CheckStateRole );
		return;
	}

	AppItem app = srcIdx.data( Roles::AppDataRole ).value<AppItem>();
	app.checked = !app.checked;
	_model.setData( srcIdx, QVariant::fromValue( app ), Roles::AppDataRole );
	_model.dataChanged( srcIdx, srcIdx, { Roles::AppDataRole } );

	toggleAppItem( srcIdx, !app.checked );
}

void MainWindow::onListDoubleClicked( const QModelIndex& idx ) {

	if( !idx.isValid() || idx.column() != 0 ) {
		return;
	}

	QStandardItem* item = _selectedListModel->itemFromIndex( idx );
	if( !item ) {
		return;
	}

	const bool checked = ( item->checkState() == Qt::Checked );
	item->setCheckState( checked ? Qt::Unchecked : Qt::Checked );
	// löst automatisch onSelectedListItemChanged aus
}

void MainWindow::onModelDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles ) {

	if( topLeft != bottomRight )
		return;

	// immer über den Index gehen (funktioniert auch in Unterordnern)
	auto nameIdx = topLeft.siblingAtColumn( COL_NAME );
	auto uacIdx = topLeft.siblingAtColumn( COL_UAC );
	auto argsIdx = topLeft.siblingAtColumn( COL_ARGS_VALUE );

	QStandardItem* nameIt = _model.itemFromIndex( nameIdx );
	QStandardItem* uacIt = _model.itemFromIndex( uacIdx );
	QStandardItem* argsIt = _model.itemFromIndex( argsIdx );

	if( !nameIt )
		return;

	if( nameIt->data( Roles::TypeRole ).toInt() != 1 )
		return;

	// Check direkt vom Item lesen – stabil bei Root & Child
	const bool checkedNow = ( nameIt->checkState() == Qt::Checked );

	AppItem app = nameIt->data( Roles::AppDataRole ).value<AppItem>();
	app.checked = checkedNow;

	if( checkedNow ) {

		if( argsIt )
			app.args = argsIt->text();

		if( uacIt )
			app.uac = ( uacIt->checkState() == Qt::Checked );
	}
	else {

		if( argsIt )
			argsIt->setText(  "" );

		if( uacIt )
			uacIt->setCheckState( Qt::Unchecked );

		app.args.clear();
		app.uac = false;
	}

	{
		QSignalBlocker block( &_model );
		nameIt->setData( QVariant::fromValue( app ), Roles::AppDataRole );
	}

	updateSelectedList();
}

void MainWindow::onSelectedListItemChanged( QStandardItem* item ) {

	if( _buildingSelected )
		return;

	if( !item )
		return;

	int row = item->row();
	QStandardItem* nameItem = _selectedListModel->item( row, 0 );
	QStandardItem* uacItem = _selectedListModel->item( row, 1 );
	QStandardItem* argsItem = _selectedListModel->item( row, 2 );

	QString path = nameItem ? nameItem->data( Qt::UserRole ).toString() : QString();
	if( path.isEmpty() )
		return;

	QStandardItem* treeItem = findTreeItemByPath( path );
	if( !treeItem )
		return;

	AppItem app = treeItem->data( Roles::AppDataRole ).value<AppItem>();

	// Reflect checkbox state
	bool checked = nameItem->checkState() == Qt::Checked;
	if( !checked ) {
		if( treeItem ) {
			treeItem->setData( Qt::Unchecked, Qt::CheckStateRole );
		}
		return;  // früh beenden -> kein Zugriff mehr auf gelöschte Items
	}

	app.checked = checked;
	treeItem->setData( checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole );

	// Reflect UAC state
	if( uacItem )
		app.uac = ( uacItem->checkState() == Qt::Checked );

	// Reflect Args
	if( argsItem )
		app.args = argsItem->text();

	// Write back to tree
	{
		QSignalBlocker block( &_model );
		treeItem->setData( QVariant::fromValue( app ), Roles::AppDataRole );
	}

	// Update the tree's UAC and Args columns visually
	QStandardItem* parent = treeItem->parent() ? treeItem->parent() : _model.invisibleRootItem();
	int treeRow = treeItem->row();
	QStandardItem* uacTreeItem = parent->child( treeRow, COL_UAC );
	QStandardItem* argsTreeItem = parent->child( treeRow, COL_ARGS_VALUE );

	if( uacTreeItem )
		uacTreeItem->setCheckState( app.uac ? Qt::Checked : Qt::Unchecked );
	if( argsTreeItem )
		argsTreeItem->setText( app.args );

	updateSelectedList();
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

	_buildingSelected = true;

	// Clear the selected list model
	_selectedListModel->clear();
	_selectedListModel->setColumnCount( 3 );
	_selectedListModel->setHorizontalHeaderLabels( { "Name", "UAC", "Args" } );

	int row = 0;

	std::function<void( QStandardItem* )> walk = [&] ( QStandardItem* it ) {

		if( !it )
			return;

		const int t = it->data( Roles::TypeRole ).toInt();

		if( t == 1 ) {

			const AppItem app = it->data( Roles::AppDataRole ).value<AppItem>();

			if( app.checked ) {

				// Name column
				auto* nameItem = new QStandardItem( app.name );
				nameItem->setEditable( false );
				nameItem->setCheckable( true );
				nameItem->setCheckState( app.checked ? Qt::Checked : Qt::Unchecked );
				nameItem->setData( app.path, Qt::UserRole );

				// UAC column
				auto* uacItem = new QStandardItem();
				uacItem->setEditable( true );
				uacItem->setCheckable( true );
				uacItem->setCheckState( app.uac ? Qt::Checked : Qt::Unchecked );

				// Args column
				auto* argsItem = new QStandardItem( app.args );
				argsItem->setEditable( true );

				_selectedListModel->setItem( row, 0, nameItem );
				_selectedListModel->setItem( row, 1, uacItem );
				_selectedListModel->setItem( row, 2, argsItem );

				++row;
			}
		}


		for( int r = 0; r < it->rowCount(); ++r )
			walk( it->child( r ) );
		};

	QStandardItem* root = _model.invisibleRootItem();

	for( int r = 0; r < root->rowCount(); ++r )
		walk( root->child( r ) );

	_buildingSelected = false;
	applyListCols();
}

QStandardItem* MainWindow::findTreeItemByPath( const QString& path ) {

	std::function<QStandardItem* ( QStandardItem* )> walk = [&] ( QStandardItem* it ) -> QStandardItem* {
		if( !it )
			return nullptr;

		if( it->data( Roles::TypeRole ).toInt() == 1 ) {
			const AppItem a = it->data( Roles::AppDataRole ).value<AppItem>();
			if( a.path.compare( path, Qt::CaseInsensitive ) == 0 )
				return it;
		}

		for( int r = 0; r < it->rowCount(); ++r ) {
			if( QStandardItem* hit = walk( it->child( r ) ) )
				return hit;
		}

		return nullptr;
		};

	QStandardItem* root = _model.invisibleRootItem();

	for( int r = 0; r < root->rowCount(); ++r ) {
		if( QStandardItem* hit = walk( root->child( r ) ) )
			return hit;
	}

	return nullptr;
}


void MainWindow::onSaveBatch() {

	const QString fileName = QFileDialog::getSaveFileName( this, "Batch speichern", QString(), "Batch (*.bat)" );
	if( fileName.isEmpty() ) {

		return;
	}

	QFile f( fileName );
	if( !f.open( QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text ) ) {

		QMessageBox::warning( this, "Fehler", "Datei kann nicht geschrieben werden." );
		return;
	}

	QTextStream out( &f );
	out << "@echo off\n";

	// 1) Brauchen wir Elevation?
	bool needsElevate{ false };
	std::function<void( QStandardItem* )> scan = [&] ( QStandardItem* it ) {

		if( it == nullptr || needsElevate ) {

			return;
		}

		if( it->data( Roles::TypeRole ).toInt() == 1 ) {

			const AppItem a = it->data( Roles::AppDataRole ).value<AppItem>();
			if( a.checked && a.uac ) {

				needsElevate = true;
				return;
			}
		}

		for( int r = 0; r < it->rowCount(); ++r ) {

			scan( it->child( r ) );
		}
		};

	if( QStandardItem* root = _model.invisibleRootItem() ) {

		for( int r = 0; r < root->rowCount() && !needsElevate; ++r ) {

			scan( root->child( r ) );
		}
	}

	// 2) Self-Elevate-Header (ein Prompt, wenn nötig)
	if( needsElevate ) {

		out << "whoami /groups | find \"S-1-16-12288\" >NUL\n"
			"if not %errorlevel%==0 (\n"
			"  powershell -NoProfile -ExecutionPolicy Bypass -Command \"Start-Process -FilePath '%~f0' -Verb RunAs\"\n"
			"  exit /b\n"
			")\n";
	}

	// 3) Escaper für cmd.exe (%, ^, \", &, |, <, >)
	auto batEscape = [] ( QString s ) -> QString {

		s.replace( "%", "%%" );
		s.replace( "^", "^^" );
		s.replace( "\"", "^\"" );
		s.replace( "&", "^&" );
		s.replace( "|", "^|" );
		s.replace( "<", "^<" );
		s.replace( ">", "^>" );
		return s;
		};

	// 4) Apps schreiben
	std::function<void( QStandardItem* )> walk = [&] ( QStandardItem* it ) {

		if( it == nullptr ) {

			return;
		}

		if( it->data( Roles::TypeRole ).toInt() == 1 ) {

			const AppItem app = it->data( Roles::AppDataRole ).value<AppItem>();
			if( app.checked ) {

				const QString safeName = app.name.trimmed().replace( '|', ' ' );
				const QString exe = batEscape( app.path );
				const QString args = batEscape( app.args );
				const QString wd = batEscape( app.workingDir );

				out << "\nREM Name=" << safeName << " | Args=" << app.args << "\n";

				if( needsElevate ) {

					if( app.uac ) {

						if( !wd.isEmpty() ) {

							out << "start \"\" /D \"" << wd << "\" \"" << exe << "\" " << args << "\n";
						}
						else {

							out << "start \"\" \"" << exe << "\" " << args << "\n";
						}
					}
					else {

						// Unelevated über Explorer-Shell
						out << "call :RunUnelevated \"" << exe << "\" \"" << args << "\" \"" << wd << "\"\n";
					}
				}
				else {
					// keine UAC-Apps insgesamt -> alles normal
					if( !wd.isEmpty() ) {

						out << "start \"\" /D \"" << wd << "\" \"" << exe << "\" " << args << "\n";
					}
					else {

						out << "start \"\" \"" << exe << "\" " << args << "\n";
					}
				}
			}
		}

		for( int r = 0; r < it->rowCount(); ++r ) {

			walk( it->child( r ) );
		}
		};

	if( QStandardItem* root2 = _model.invisibleRootItem() ) {

		for( int r = 0; r < root2->rowCount(); ++r ) {

			walk( root2->child( r ) );
		}
	}

	// 5) Batch beenden + Helper nur falls elevated gebraucht wird
	if( needsElevate ) {

		out << "\nexit\n"
			"goto :eof\n"
			"\n:RunUnelevated\n"
			"set \"exe=%~1\"\n"
			"set \"args=%~2\"\n"
			"set \"wd=%~3\"\n"
			"powershell -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -Command ^\n"
			" \"$exe='%exe%'; $args='%args%'; $wd='%wd%';"
			"  if([string]::IsNullOrWhiteSpace($args)){$args=$null};"
			"  if([string]::IsNullOrWhiteSpace($wd)){$wd=$null};"
			"  $shell = New-Object -ComObject Shell.Application;"
			"  $shell.ShellExecute($exe, $args, $wd, 'open', 1)\" ^\n"
			" \n"
			"exit /b\n";

	}
	else {

		out << "\nexit\n";
	}
}

void MainWindow::onLoadBatch() {

	const QString fileName = QFileDialog::getOpenFileName( this, "Batch laden", QString(), "Batch (*.bat)" );
	if( fileName.isEmpty() ) return;

	QFile f( fileName );
	if( !f.open( QIODevice::ReadOnly | QIODevice::Text ) ) return;
	QTextStream in( &f );

	struct Entry {
		QString name, path, workingdir, args;
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
					QModelIndex src = _model.indexFromItem( it );
					for( QModelIndex p = src.parent(); p.isValid(); p = p.parent() ) {
						const QModelIndex prox = _proxy->mapFromSource( p );
						if( prox.isValid() ) {
							_tree->setExpanded( prox, true );
						}
					}
					found = true;
					return;
				}
			}
			for( int r = 0; r < it->rowCount(); ++r ) find( it->child( r ) );
			};
		for( int r = 0; r < root->rowCount() && !found; ++r ) find( root->child( r ) );

		if( !found ) {
			// Kein (external)-Ordner: direkt ins Root einfügen
			QStandardItem* parent = _model.invisibleRootItem(); // oder: ensureFolderPath("")

			AppItem app{
				e.name.isEmpty() ? QFileInfo( e.path ).completeBaseName() : e.name,
				e.path,
				e.workingdir,
				e.args,
				true            // checked
				// , /* optional: uac */ false
			};

			// Spalte 0: Name + Checkbox
			auto* nameIt = new QStandardItem( app.name );
			nameIt->setEditable( false );
			nameIt->setFlags( nameIt->flags() | Qt::ItemIsUserCheckable );
			nameIt->setData( Qt::Checked, Qt::CheckStateRole );
			nameIt->setData( 1, Roles::TypeRole );
			nameIt->setData( QVariant::fromValue( app ), Roles::AppDataRole );

			// Spalte 1: UAC-Checkbox (falls du AppItem::uac nutzt – sonst bleibt unchecked)
			auto* uacIt = new QStandardItem();
			uacIt->setEditable( false );
			uacIt->setFlags( uacIt->flags() | Qt::ItemIsUserCheckable );
			uacIt->setData( Qt::Unchecked, Qt::CheckStateRole );

			// Spalte 2: Args (editierbar)
			auto* argsIt = new QStandardItem( app.args );
			argsIt->setEditable( true );

			parent->appendRow( { nameIt, uacIt, argsIt } );

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
				QModelIndex src = _model.indexFromItem( it );
				for( QModelIndex p = src.parent(); p.isValid(); p = p.parent() ) {
					const QModelIndex prox = _proxy->mapFromSource( p );
					if( prox.isValid() ) {
						_tree->setExpanded( prox, true );
					}
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
		AppItem app{ QFileInfo( file ).completeBaseName(), file, QString(), QString(), true };
		auto* nameIt = new QStandardItem( app.name );
		nameIt->setEditable( false );
		nameIt->setFlags( nameIt->flags() | Qt::ItemIsUserCheckable );
		nameIt->setData( Qt::Checked, Qt::CheckStateRole );
		nameIt->setData( 1, Roles::TypeRole );
		nameIt->setData( QVariant::fromValue( app ), Roles::AppDataRole );

		auto* uacIt = new QStandardItem();
		uacIt->setEditable( false );
		uacIt->setFlags( uacIt->flags() | Qt::ItemIsUserCheckable );
		uacIt->setData( Qt::Unchecked, Qt::CheckStateRole );

		auto* argsIt = new QStandardItem( app.args );
		argsIt->setEditable( true );

		ext->appendRow( { nameIt, uacIt, argsIt } );		_pathIndex.insert( app.path.toLower(), app );
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