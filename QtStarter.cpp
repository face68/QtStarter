#include <QApplication>
#include "MainWindow.h"

int main( int argc, char* argv[] ) {
	QApplication app( argc, argv );
	QApplication::setApplicationName( "StarterQt" );
	QApplication::setOrganizationName( "Starter" );

	MainWindow w;
	w.resize( 900, 600 );
	w.setWindowTitle( "Batch Launcher (Qt)" );
	w.show();
	return app.exec();
}