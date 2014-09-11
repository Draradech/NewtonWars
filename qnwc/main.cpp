#include "MainWindow.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
   QApplication a( argc, argv );
   a.setOrganizationName( "xayax.net" );
   a.setApplicationName( "QNewtonWarsClient" );
   MainWindow w;
   w.show();

   return a.exec();
}
