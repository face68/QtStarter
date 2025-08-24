#pragma once
#include <QMainWindow>
#include <QStandardItemModel>
#include <QTreeView>
#include <QTableView>
#include <QPointer>
#include "AppItem.h"
#include <QSortFilterProxyModel>
#include <QLineEdit>

class MainWindow: public QMainWindow {

	Q_OBJECT
	public:
		explicit MainWindow( QWidget* parent = nullptr );

	protected:
		void changeEvent( QEvent* e ) override;
		void resizeEvent( QResizeEvent* e ) override;

	private slots:
		void onTreeDoubleClicked( const QModelIndex& idx );
		void onListDoubleClicked( const QModelIndex& idx );
		void onSaveBatch();
		void onLoadBatch();
		void onAddExecutable();
		void onModelDataChanged( const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>() );
		void onSelectedListItemChanged( QStandardItem* item );
		void onSearchTextChanged( const QString& text );

	private:
		void buildUi();
		void setStyleSheets();
		void applyListCols();
		QString getRelativeFolder( const QString& root, const QString& fullPath ) const;
		void scanStartMenu();
		QStandardItem* ensureFolderPath( const QString& relFolder );
		QStandardItem* findTreeItemByPath( const QString& path );
		void toggleAppItem( const QModelIndex& idx, bool checked );

		void updateSelectedList();

		// helpers batch parsing
		static QString extractQuotedPath( const QString& line );
		static QString extractArgsAfterQuotedPath( const QString& line );
		static QString parseRemName( const QString& remLine );

		private:
		QPointer<QTreeView>				_tree;
		QPointer<QTableView>			_selectedListView;
		QPointer<QStandardItemModel>	_selectedListModel;
		QStandardItemModel				_model;
		QPointer<QLineEdit>				_searchEdit;
		QSortFilterProxyModel*			_proxy;
		// fast lookup by path
		QHash<QString, App::AppItem>			_pathIndex; // key: exe path (stored lower-cased)
		bool							_buildingSelected;
		bool							_stylingNow{ false };
};