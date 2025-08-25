#include "AppListDelegate.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QCheckBox>
#include <QLineEdit>
#include <QMouseEvent>
#include "AppItem.h"

void AppListDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const {

	painter->save();

	QStyleOptionViewItem opt( option );
	initStyleOption( &opt, index );
	opt.state &= ~QStyle::State_HasFocus;
	opt.state &= ~QStyle::State_Selected;

	QStyle* st = QApplication::style();
	QPalette pal = opt.palette;

	QRect rect = option.rect;
	const int margin = 4;
	const int spacing = 8;

	if( index.column() == 0 ) {

		// Checkbox + Name (ein Feld)
		QStyleOptionButton cb;
		cb.palette = pal;
		cb.state = QStyle::State_Enabled
			| ( index.data( Qt::CheckStateRole ).toInt() == Qt::Checked ? QStyle::State_On : QStyle::State_Off );

		const int w = st->pixelMetric( QStyle::PM_IndicatorWidth, &opt, opt.widget );
		const int h = st->pixelMetric( QStyle::PM_IndicatorHeight, &opt, opt.widget );
		const QRect cbRect( rect.left() + margin, rect.top() + ( rect.height() - h ) / 2, w, h );

		cb.rect = cbRect;
		st->drawControl( QStyle::CE_CheckBox, &cb, painter, opt.widget );

		// Text rechts neben der Checkbox
		const QString name = index.data( Qt::DisplayRole ).toString();
		const QRect tr( cbRect.right() + spacing, rect.top(), rect.right() - ( cbRect.right() + spacing ) - margin, rect.height() );
		const bool selected = ( opt.state & QStyle::State_Selected ) != 0;

		st->drawItemText( painter, tr, Qt::AlignVCenter | Qt::AlignLeft, pal, true, name, QPalette::Text );
	}
	else if( index.column() == 1 ) {

		const bool manual = index.sibling( index.row(), 0 ).data( Roles::ManualRole ).toBool();
		if( !manual ) {
			painter->restore();
			return;
		}

		QPalette pal = opt.palette;
		pal.setCurrentColorGroup( QPalette::Active );

		QColor win = pal.color( QPalette::Window );
		const bool light = win.lightness() > 127 ? true : false;
		const bool alt = ( opt.features & QStyleOptionViewItem::Alternate );

		//            alt ? ( light ? lighter alt           : darker alt              : ( light ? lighter non alt       : darker non alt
		QColor base = alt ? ( light ? QColor( 0, 0, 0, 30 ) : QColor( 0, 0, 0, 30 ) ) : ( light ? QColor( 0, 0, 0, 20 ) : QColor( 0, 0, 0, 30 ) );

		QRect cell = option.rect;
		if( auto view = qobject_cast< const QAbstractItemView* >( opt.widget ) ) {

			cell = view->visualRect( index );
		}

		QRect r = cell.adjusted( 2, 4, -4, -4 );
		painter->fillRect( r, base );

		// optional: hauchdünner Rand
		painter->setPen( QColor( 0, 0, 0, 20 ) );
		painter->drawLine( r.topLeft(), r.topRight() );
		painter->drawLine( r.topLeft(), r.bottomLeft() );
		painter->setPen( QColor( 255, 255, 255, 20 ) );
		painter->drawLine( r.bottomLeft(), r.bottomRight() );
		painter->drawLine( r.bottomRight(), r.topRight() );

		// Text reinschreiben (mit Padding)
		const QRect tr = r.adjusted( 6, 0, -6, 0 );
		QStyle* st = opt.widget ? opt.widget->style() : QApplication::style();
		st->drawItemText( painter, tr, Qt::AlignVCenter | Qt::AlignLeft, pal, true, index.data( Qt::DisplayRole ).toString(), QPalette::Text );
	}
	else if( index.column() == 2 ) {

		QStyleOptionButton uac;
		uac.palette = opt.palette;
		uac.state = QStyle::State_Enabled | ( index.data( Qt::CheckStateRole ).toInt() == Qt::Checked ? QStyle::State_On : QStyle::State_Off );

		const int w = st->pixelMetric( QStyle::PM_IndicatorWidth, &opt, opt.widget );
		const int h = st->pixelMetric( QStyle::PM_IndicatorHeight, &opt, opt.widget );
		uac.rect = QRect( option.rect.left() + ( option.rect.width() - w ) / 2, option.rect.top() + ( option.rect.height() - h ) / 2, w, h );
		st->drawControl( QStyle::CE_CheckBox, &uac, painter, opt.widget );
	}
	else if( index.column() == 3 ) {

		QPalette pal = opt.palette;
		pal.setCurrentColorGroup( QPalette::Active );

		QColor win = pal.color( QPalette::Window );
		const bool light = win.lightness() > 127 ? true : false;
		const bool alt = ( opt.features & QStyleOptionViewItem::Alternate );

		//            alt ? ( light ? lighter alt           : darker alt              : ( light ? lighter non alt       : darker non alt
		QColor base = alt ? ( light ? QColor( 0, 0, 0, 30 ) : QColor( 0, 0, 0, 30 ) ) : ( light ? QColor( 0, 0, 0, 20 ) : QColor( 0, 0, 0, 30 ) );

		QRect cell = option.rect;
		if( auto view = qobject_cast< const QAbstractItemView* >( opt.widget ) ) {

			cell = view->visualRect( index ); 
		}

		QRect r = cell.adjusted( 2, 4, -4, -4 );
		painter->fillRect( r, base );

		// optional: hauchdünner Rand
		painter->setPen( QColor( 0, 0, 0, 20 ) );
		painter->drawLine( r.topLeft(), r.topRight() );
		painter->drawLine( r.topLeft(), r.bottomLeft() );
		painter->setPen( QColor( 255, 255, 255, 20 ) );
		painter->drawLine( r.bottomLeft(), r.bottomRight() );
		painter->drawLine( r.bottomRight(), r.topRight() );

		// Text reinschreiben (mit Padding)
		const QRect tr = r.adjusted( 6, 0, -6, 0 );
		QStyle* st = opt.widget ? opt.widget->style() : QApplication::style();
		st->drawItemText( painter, tr, Qt::AlignVCenter | Qt::AlignLeft, pal, true, index.data( Qt::DisplayRole ).toString(), QPalette::Text );
	}

	painter->restore();
}

QSize AppListDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const {

	QSize s = QStyledItemDelegate::sizeHint( option, index );

	QStyle* st = QApplication::style();
	const int h = st->pixelMetric( QStyle::PM_IndicatorHeight, nullptr, nullptr );
	if( s.height() < h + 8 ) {
		s.setHeight( h + 8 );
	}

	return s;
}

QWidget* AppListDelegate::createEditor( QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index ) const {

	if( index.column() == 1 ) {

		if( index.sibling( index.row(), 0 ).data( Roles::ManualRole ).toBool() )
			return new QLineEdit( parent );

		return nullptr;
	}

	if( index.column() == 3 ) {

		return new QLineEdit( parent );
	}
	return nullptr;
}

void AppListDelegate::setEditorData( QWidget* editor, const QModelIndex& index ) const {

	if( auto* line = qobject_cast< QLineEdit* >( editor ) ) {
		line->setText( index.data( Qt::DisplayRole ).toString() );
	}
}

void AppListDelegate::setModelData( QWidget* editor, QAbstractItemModel* model, const QModelIndex& index ) const {

	if( auto* line = qobject_cast< QLineEdit* >( editor ) ) {
		model->setData( index, line->text(), Qt::EditRole );
	}
}

bool AppListDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index ) {

	if( event->type() == QEvent::MouseButtonRelease ) {

		const QRect rect = option.rect;
		const int margin = 4;
		QStyle* st = QApplication::style();

		const int w = st->pixelMetric( QStyle::PM_IndicatorWidth, nullptr, nullptr );
		const int h = st->pixelMetric( QStyle::PM_IndicatorHeight, nullptr, nullptr );
		const QRect checkRect( option.rect.left() + ( option.rect.width() - w ) / 2,
							   option.rect.top() + ( option.rect.height() - h ) / 2,
							   w, h );

		auto* me = static_cast< QMouseEvent* >( event );

		if( ( index.column() == 0 || index.column() == 2 ) && checkRect.contains( me->pos() ) ) {

			const bool on = index.data( Qt::CheckStateRole ).toInt() == Qt::Checked;
			model->setData( index, on ? Qt::Unchecked : Qt::Checked, Qt::CheckStateRole );
			return true;
		}
	}

	return QStyledItemDelegate::editorEvent( event, model, option, index );
}
