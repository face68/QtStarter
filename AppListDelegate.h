#pragma once

#include "AppItem.h"

#include <QStyledItemDelegate>
#include <QPainter>
#include <qabstractitemview.h>

#include <QDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QApplication>
#include <QPainterPath>

enum Col {
	COL_NAME = 0, COL_UAC, COL_ARGS_VALUE, COL__COUNT
};

class AppListDelegate: public QStyledItemDelegate {

	Q_OBJECT

	public:
		using QStyledItemDelegate::QStyledItemDelegate;
		void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
		QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
		QWidget* createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;
		void setEditorData( QWidget* editor, const QModelIndex& index ) const override;
		void setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const override;
		bool editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index ) override;
};

class NoHoverDelegate: public QStyledItemDelegate {

	Q_OBJECT

	public:
		NoHoverDelegate( int h, QObject* p = nullptr ) : QStyledItemDelegate( p ), H( h ) {}
		QSize sizeHint( const QStyleOptionViewItem& o, const QModelIndex& i ) const override {

			QSize s = QStyledItemDelegate::sizeHint( o, i );
			if( s.height() < H ) {

				s.setHeight( H );
			}
			return s;
		}
		void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override {

			if( index.column() != COL_ARGS_VALUE || option.state & QStyle::State_MouseOver ) {

				return QStyledItemDelegate::paint( painter, option, index );
			}

			QStyleOptionViewItem opt( option );

			const QModelIndex nameIdx = index.sibling( index.row(), COL_NAME );
			const int type = nameIdx.data( Roles::TypeRole ).toInt();

			if( type == 0 )
				return QStyledItemDelegate::paint( painter, option, index ); // nur für Apps


			QPalette pal = opt.palette;
			pal.setCurrentColorGroup( QPalette::Active );

			QColor win = pal.color( QPalette::Window );
			const bool light = win.lightness() > 127 ? true : false;
			const bool alt = ( opt.features & QStyleOptionViewItem::Alternate );
			//            alt ? ( light ? lighter alt           : darker alt              : ( light ? lighter non alt       : darker non alt
			QColor base = alt ? ( light ? QColor( 0, 0, 0, 30 ) : QColor( 0, 0, 0, 30 ) ) : ( light ? QColor( 0, 0, 0, 20 ) : QColor( 0, 0, 0, 30 ) );

			QRect cell = option.rect;
			if( auto view = qobject_cast< const QAbstractItemView* >( opt.widget ) ) {

				cell = view->visualRect( index ); // garantiert die Zelle, nicht die ganze Zeile
			}

			QRect r = cell.adjusted( 2, 4, -4, -4 );
			painter->fillRect( r, base );

			//r = cell.adjusted( 2, 4, -3, -2 );

			// optional: hauchdünner Rand
			painter->setPen( QColor( 0, 0, 0, 40 ) );
			painter->drawLine( r.topLeft(), r.topRight() );
			painter->drawLine( r.topLeft(), r.bottomLeft() );
			painter->setPen( QColor( 255, 255, 255, 40 ) );
			painter->drawLine( r.bottomLeft(), r.bottomRight() );
			painter->drawLine( r.bottomRight(), r.topRight() );

			// Text reinschreiben (mit Padding)
			const QRect tr = r.adjusted( 6, 0, -6, 0 );
			QStyle* st = opt.widget ? opt.widget->style() : QApplication::style();
			st->drawItemText( painter, tr, Qt::AlignVCenter | Qt::AlignLeft,
							  pal, true, index.data( Qt::DisplayRole ).toString(), QPalette::Text );
		}


	private:
		int H;
};