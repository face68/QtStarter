#pragma once
#include <QSortFilterProxyModel>
#include "AppItem.h"  // enthält Roles::TypeRole (0=Folder, 1=App)

class AppFilterProxyModel: public QSortFilterProxyModel {
	Q_OBJECT
	public:
	using QSortFilterProxyModel::QSortFilterProxyModel;

	void setAppTypeValue( int v ) {
		_appType = v;
	}

	protected:
	bool filterAcceptsRow( int source_row, const QModelIndex& source_parent ) const override {

		if( filterRegularExpression().pattern().isEmpty() ) {
			return true;
		}

		const QModelIndex idx = sourceModel()->index( source_row, filterKeyColumn(), source_parent );
		const int t = idx.data( Roles::TypeRole ).toInt();

		// Nur Apps werden gegen den Regex geprüft
		if( t == _appType ) {
			return QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent );
		}

		// Ordner bleiben nur, wenn ein Kind akzeptiert wird
		const int childCount = sourceModel()->rowCount( idx );
		for( int r = 0; r < childCount; ++r ) {
			if( filterAcceptsRow( r, idx ) ) {
				return true;
			}
		}

		return false;
	}

	private:
	int _appType{ 1 }; // 1 = App (laut AppItem.h)
};