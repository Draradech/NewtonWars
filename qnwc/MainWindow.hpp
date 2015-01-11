#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

class MainWidget;

class MainWindow : public QMainWindow
{
   Q_OBJECT

public:
   MainWindow( QWidget *parent = 0 );
   ~MainWindow();

public slots:

signals:

private:
   MainWidget        *mpMainWidget;
};

#endif // MAINWINDOW_HPP
