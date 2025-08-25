#include <QApplication>
#include "MainWindow.h"

#define BS_VERSION       "0.1.0"
#define BS_VERSION_FULL  "Batch Starter v0.1.0"

int main( int argc, char* argv[] ) {

	QApplication app( argc, argv );
	QApplication::setApplicationName( "StarterQt" );
	QApplication::setOrganizationName( "Starter" );

	MainWindow w;
	w.resize( 900, 600 );
	w.setWindowTitle( QString( "%1" ).arg( BS_VERSION_FULL ) );
	w.show();
	return app.exec();
}