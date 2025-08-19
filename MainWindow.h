#pragma once
#include <QMainWindow>
#include <QStandardItemModel>
#include <QTreeView>
#include <QListWidget>
#include <QPointer>
#include "AppItem.h"

class MainWindow: public QMainWindow {

	Q_OBJECT
	public:
		explicit MainWindow( QWidget* parent = nullptr );

	private slots:
		void onTreeDoubleClicked( const QModelIndex& idx );
		void onSaveBatch();
		void onLoadBatch();
		void onAddExecutable();
		void onModelDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>() );
		void onSelectedListItemChanged( QListWidgetItem* item );

	private:
		void buildUi();
		QString getRelativeFolder( const QString& root, const QString& fullPath ) const;
		void scanStartMenu();
		QStandardItem* ensureFolderPath( const QString& relFolder );
		void toggleAppItem( const QModelIndex& idx, bool checked );

		void updateSelectedList();

		// helpers batch parsing
		static QString extractQuotedPath( const QString& line );
		static QString extractArgsAfterQuotedPath( const QString& line );
		static QString parseRemName( const QString& remLine );

		private:
		QPointer<QTreeView>     _tree;
		QPointer<QListWidget>   _selectedList;
		QStandardItemModel      _model;

		// fast lookup by path
		QHash<QString, AppItem> _pathIndex; // key: exe path (stored lower-cased)
};