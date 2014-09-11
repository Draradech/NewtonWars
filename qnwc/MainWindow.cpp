#include "MainWindow.hpp"

#include <QSplitter>

#include "MainWidget.hpp"


MainWindow::MainWindow( QWidget *parent )
: QMainWindow( parent )
, mpMainWidget( new MainWidget( this ) )
{
   setCentralWidget( mpMainWidget );
}


MainWindow::~MainWindow()
{
}
