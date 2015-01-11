#ifndef MAINWIDGET_HPP
#define MAINWIDGET_HPP

#include <QWidget>

#include <QScopedPointer>
#include <QTcpSocket>
#include <QThread>

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSpinBox;

class TcpWorker;
class AngleDrawWidget;

class MainWidget : public QWidget
{
   Q_OBJECT
public:
   explicit MainWidget( QWidget *parent = 0 );
   ~MainWidget();

signals:
   void sendToSocket( const QByteArray &msg );
   void angleChanged( double angle );

public slots:
   void setTexts();
   void setAnglePrecision( int precision );
   void setPowerPrecision( int precision );
   void onConnected( bool connected = true );
   void onDisconnected();
   void initiateConnection();
   void sendShotParameters();
   void sendClear();
   void sanitizeAngle( double value );
   void sanitizePower( double value );
   void addToLog( const QByteArray &msg );
   void addToLog( const QString &msg );
   void nameChanged();
   void nameUpdate();
   void angleChanged(const QString &newAngle);
   void socketState(QAbstractSocket::SocketState state);

private:
   QTimer                   *mpTimerName;
   QLabel                   *mpServerLabel;
   QLineEdit                *mpServerName;
   QSpinBox                 *mpServerPort;
   QPushButton              *mpConnectButton;
   QPushButton              *mpDisconnectButton;
   QLabel                   *mpPlayerLabel;
   QLineEdit                *mpPlayerName;
   QSpinBox                 *mpPlayerWidth;
   QLabel                   *mpAngleLabel;
   QLabel                   *mpPowerLabel;
   QDoubleSpinBox           *mpAngleSpinBox;
   QDoubleSpinBox           *mpPowerSpinBox;
   QSpinBox                 *mpAnglePrecision;
   QSpinBox                 *mpPowerPrecision;
   QLineEdit                *mpAngleEdit;
   QLineEdit                *mpPowerEdit;
   QPushButton              *mpFireButton;
   QPushButton              *mpClearButton;
   QListWidget              *mpMessageBuffer;
   AngleDrawWidget          *mpAngleDrawWidget;
   int                       mInNamePos;
   //QScopedPointer<QThread>   mTcpThread;
   QThread                   mTcpThread;
   TcpWorker                *mpTcpWorker;
};

#endif // MAINWIDGET_HPP
