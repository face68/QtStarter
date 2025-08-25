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


		const QModelIndex idx = sourceModel()->index( source_row, filterKeyColumn(), source_parent );
		const int t = idx.data( Roles::TypeRole ).toInt();
		const bool isManual = idx.data( Roles::ManualRole ).toBool();

		// 1) Auch ohne Suchtext: manuelle Apps verbergen
		if( filterRegularExpression().pattern().isEmpty() ) {
			if( t == _appType ) {
				return !isManual;           // Apps: nur zeigen, wenn NICHT manuell
			}
			return true;                    // Ordner immer zeigen
		}

		// 2) Mit Suchtext:
		if( t == _appType ) {
			if( isManual ) {
				return false;               // manuelle Apps nie im Tree
			}
			return QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent );
		}

		// Ordner bleibt, wenn ein Kind akzeptiert wird
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