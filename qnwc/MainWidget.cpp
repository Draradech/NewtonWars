#include "MainWidget.hpp"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QSpinBox>
#include <QThread>

#include <QDateTime>
#include <QTimer>

#include <QTcpSocket>

#include "TCPWorker.hpp"
#include "AngleDrawWidget.hpp"
#include "MyDoubleSpinBox.hpp"


MainWidget::MainWidget( QWidget *parent )
: QWidget( parent )
, mpTimerName(        new QTimer(this) )
, mpServerLabel(      new QLabel(this) )
, mpServerName(       new QLineEdit(this) )
, mpServerPort(       new QSpinBox(this) )
, mpConnectButton(    new QPushButton(this) )
, mpDisconnectButton( new QPushButton(this) )
, mpPlayerLabel(      new QLabel(this) )
, mpPlayerName(       new QLineEdit(this) )
, mpPlayerWidth(      new QSpinBox(this) )
, mpAngleLabel(       new QLabel(this) )
, mpPowerLabel(       new QLabel(this) )
, mpAngleSpinBox(     new MyDoubleSpinBox(this) )
, mpPowerSpinBox(     new MyDoubleSpinBox(this) )
, mpAnglePrecision(   new QSpinBox(this) )
, mpPowerPrecision(   new QSpinBox(this) )
, mpAngleEdit(        new QLineEdit(this) )
, mpPowerEdit(        new QLineEdit(this) )
, mpFireButton(       new QPushButton(this) )
, mpClearButton(      new QPushButton(this) )
, mpMessageBuffer(    new QListWidget(this) )
, mpAngleDrawWidget(  new AngleDrawWidget(this) )
, mInNamePos(         0 )
//, mTcpThread(         new QThread(0) )
, mTcpThread(         0 )
, mpTcpWorker(        new TcpWorker(&mTcpThread) )
{
   mpAnglePrecision->setAlignment( Qt::AlignRight );
   mpPowerPrecision->setAlignment( Qt::AlignRight );
   mpFireButton->setShortcut( QKeySequence("Enter") );

   QGridLayout *layout( new QGridLayout(this) );
   layout->setColumnStretch( 1, 1 );

   QSplitter *splitter = new QSplitter( this );
   splitter->addWidget( mpMessageBuffer );
   splitter->addWidget( mpAngleDrawWidget );

   layout->addWidget( mpServerLabel,      0, 0 );
   layout->addWidget( mpServerName,       0, 1 );
   layout->addWidget( mpServerPort,       0, 2 );
   layout->addWidget( mpConnectButton,    0, 3 );
   layout->addWidget( mpDisconnectButton, 0, 3 );

   layout->addWidget( mpPlayerLabel,      1, 0 );
   layout->addWidget( mpPlayerName,       1, 1, 1, 2 );
   layout->addWidget( mpPlayerWidth,      1, 3 );

   layout->addWidget( mpAngleLabel,       2, 0 );
   layout->addWidget( mpAngleSpinBox,     2, 1 );
   layout->addWidget( mpAnglePrecision,   2, 2 );
   layout->addWidget( mpAngleEdit,        2, 3 );

   layout->addWidget( mpPowerLabel,       3, 0 );
   layout->addWidget( mpPowerSpinBox,     3, 1 );
   layout->addWidget( mpPowerPrecision,   3, 2 );
   layout->addWidget( mpPowerEdit,        3, 3 );

   layout->addWidget( mpClearButton,      4, 0 );
   layout->addWidget( mpFireButton,       4, 1, 1, 3 );
   layout->addWidget( splitter,           5, 0, 1, 4 );

   connect( mpAnglePrecision, SIGNAL(valueChanged(int)),
            this, SLOT(setAnglePrecision(int)) );
   connect( mpAngleSpinBox, SIGNAL(valueChanged(double)),
            this, SLOT(sanitizeAngle(double)) );

   connect( mpPowerPrecision, SIGNAL(valueChanged(int)),
            this, SLOT(setPowerPrecision(int)) );
   connect( mpPowerSpinBox, SIGNAL(valueChanged(double)),
            this, SLOT(sanitizePower(double)) );

   connect( mpTcpWorker, SIGNAL(connected()),
            this, SLOT(onConnected()) );
   connect( mpTcpWorker, SIGNAL(disconnected()),
            this, SLOT(onDisconnected()) );
   connect( this, SIGNAL(sendToSocket(QByteArray)),
            mpTcpWorker, SLOT(send(QByteArray)) );
   connect( mpTcpWorker, SIGNAL(received(QByteArray)),
            this, SLOT(addToLog(QByteArray)) );
   connect( mpTcpWorker, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SLOT(socketState(QAbstractSocket::SocketState)) );

   connect( mpConnectButton, SIGNAL(clicked()),
            this, SLOT(initiateConnection()) );
   connect( mpDisconnectButton, SIGNAL(clicked()),
            mpTcpWorker, SLOT(close()) );
   connect( mpClearButton, SIGNAL(clicked()),
            this, SLOT(sendClear()) );
   connect( mpFireButton, SIGNAL(clicked()),
            this, SLOT(sendShotParameters()) );
   connect( mpAngleEdit, SIGNAL(returnPressed()),
            this, SLOT(sendShotParameters()) );
   connect( mpPowerEdit, SIGNAL(returnPressed()),
            this, SLOT(sendShotParameters()) );
   connect( mpAngleSpinBox, SIGNAL(returnPressed()),
            this, SLOT(sendShotParameters()) );
   connect( mpPowerSpinBox, SIGNAL(returnPressed()),
            this, SLOT(sendShotParameters()) );

   connect( mpPlayerName, SIGNAL(textEdited(QString)),
            this, SLOT(nameChanged()) );
   connect( mpPlayerWidth, SIGNAL(valueChanged(int)),
            this, SLOT(nameChanged()) );
   connect( mpTimerName, SIGNAL(timeout()),
            this, SLOT(nameUpdate()) );

   connect( mpAngleEdit, SIGNAL(textChanged(QString)),
            this, SLOT(angleChanged(QString)) );
   connect( this, SIGNAL(angleChanged(double)),
            mpAngleDrawWidget, SLOT(setAngle(double)) );

   //mTcpThread->start();
   mTcpThread.start();
   setLayout( layout );

   mpServerName->setText( "10.23.42.143" );
   mpServerPort->setRange( 0, 65535 );
   mpServerPort->setValue( 3490 );

   mpAnglePrecision->setValue( 0 );
   mpAnglePrecision->setRange( 0, 15 );
   mpAngleSpinBox->setRange( -360.0, 360.0 );
   mpAngleEdit->setText( "0" ); // workaround

   mpPowerPrecision->setValue( 1 );
   mpPowerPrecision->setRange( 0, 15 );
   mpPowerSpinBox->setRange( 0.0, 15.0 );
   mpPowerSpinBox->setValue( 0.1 );
   mpPowerSpinBox->setValue( 10.0 );

   setAnglePrecision( mpAnglePrecision->value() );
   setPowerPrecision( mpPowerPrecision->value() );
   setTexts();
   onDisconnected();
   //onConnected(); // enable only for debugging
   setMinimumWidth( 400 );
}


MainWidget::~MainWidget()
{
   QMetaObject::invokeMethod( &mTcpThread, "quit" );
   while( !mTcpThread.isFinished() )
   {
      QApplication::processEvents();
   }
}


void MainWidget::setTexts()
{
   mpServerLabel->setText(      tr("Server/Port:") );
   mpConnectButton->setText(    tr("Connect") );
   mpDisconnectButton->setText( tr("Disconnect") );
   mpClearButton->setText(      tr("Clear") );
   mpPlayerLabel->setText(      tr("Player Name:") );
   mpAngleLabel->setText(       tr("Angle:") );
   mpPowerLabel->setText(       tr("Velocity:") );
   mpFireButton->setText(       tr("Fire") );
}


void MainWidget::setAnglePrecision( int precision )
{
   mpAngleSpinBox->setDecimals( precision );
   long double step;
   for( step = 1.0; precision; --precision )
   {
      step /= 10;
   }
   mpAngleSpinBox->setSingleStep( step );
}


void MainWidget::setPowerPrecision( int precision )
{
   mpPowerSpinBox->setDecimals( precision );
   long double step;
   for( step = 1.0; precision; --precision )
   {
      step /= 10;
   }
   mpPowerSpinBox->setSingleStep( step );
}


void MainWidget::onConnected( bool connected )
{
   mpConnectButton->setVisible(    !connected );
   mpDisconnectButton->setVisible(  connected );
   mpAngleLabel->setEnabled(        connected );
   mpAngleSpinBox->setEnabled(      connected );
   mpAnglePrecision->setEnabled(    connected );
   mpAngleEdit->setEnabled(         connected );
   mpPowerLabel->setEnabled(        connected );
   mpPowerSpinBox->setEnabled(      connected );
   mpPowerPrecision->setEnabled(    connected );
   mpPowerEdit->setEnabled(         connected );
   mpFireButton->setEnabled(        connected );
   mpClearButton->setEnabled(       connected );
   if( connected )
   {
      nameChanged();
   }
}


void MainWidget::onDisconnected()
{
   onConnected( false );
}


void MainWidget::initiateConnection()
{
   QMetaObject::invokeMethod( mpTcpWorker, "connectToHost",
                              Q_ARG(QString,mpServerName->text()),
                              Q_ARG(int,mpServerPort->value()));
}


void MainWidget::sendShotParameters()
{
   QByteArray data("v ");
   data.append( mpPowerEdit->text() );
   data.append( '\n' );
   data.append( mpAngleEdit->text() );
   data.append( '\n' );

   emit sendToSocket( data );
   data.prepend( "Sending:\n" );
   addToLog( data.trimmed() );
}


void MainWidget::sendClear()
{
   emit sendToSocket( QByteArray("c\n") );
}


void MainWidget::sanitizeAngle( double value )
{
   mpAngleEdit->setText( QString::number( value ).replace( ',', '.' ) );
}


void MainWidget::sanitizePower( double value )
{
   mpPowerEdit->setText( QString::number( value ).replace( ',', '.' ) );
}


void MainWidget::addToLog( const QByteArray &msg )
{
   addToLog( QString::fromUtf8( msg ) );
}


void MainWidget::addToLog( const QString &msg )
{
   QListWidgetItem *item = new QListWidgetItem( QDateTime::currentDateTime().toString(), mpMessageBuffer );
   item->setBackground( QBrush( mpMessageBuffer->palette().color( QPalette::AlternateBase ) ) );
   mpMessageBuffer->addItem( item );
   mpMessageBuffer->addItem( msg );

   while( mpMessageBuffer->count() > 1000 )
   {
      item = mpMessageBuffer->takeItem(0);
      if( item )
      {
         delete item;
      }
   }

   mpMessageBuffer->scrollToBottom();
}


void MainWidget::nameChanged()
{
   if( mpPlayerWidth->value() > 0 )
   {
      mpTimerName->setSingleShot( false );
      mpTimerName->setInterval( 333 );
      mpTimerName->start();
   }
   else
   {
      nameUpdate();
      mpTimerName->stop();
   }
}


void MainWidget::nameUpdate()
{
   QString name( mpPlayerName->text() );

   if( mpPlayerWidth->value() > 0 )
   {
      name.prepend( QString( mpPlayerWidth->value(), ' ' ) );
      if( (++mInNamePos) >= name.size() )
      {
         mInNamePos = 0;
      }
      name.append( QString( mpPlayerWidth->value(), ' ' ) );
      name = name.mid( mInNamePos, mpPlayerWidth->value() );
   }

   name.prepend( "n " );
   name.append( '\n' );
   //emit addToLog( QString("%1").arg(name) );
   emit sendToSocket( name.toUtf8() );
}


void MainWidget::angleChanged( const QString &newAngle )
{
   emit angleChanged( newAngle.toDouble() );
}


void MainWidget::socketState(QAbstractSocket::SocketState state)
{
   mpConnectButton->setEnabled( state == QAbstractSocket::UnconnectedState );
   switch( state )
   {
   case QAbstractSocket::UnconnectedState:
      addToLog( tr("unconnected") );
      break;
   case QAbstractSocket::HostLookupState:
      addToLog( tr("host lookup") );
      break;
   case QAbstractSocket::ConnectingState:
      addToLog( tr("connecting") );
      break;
   case QAbstractSocket::ConnectedState:
      addToLog( tr("connected") );
      break;
   default:
      break;
   }
}
