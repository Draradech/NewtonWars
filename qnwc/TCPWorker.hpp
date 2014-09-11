#ifndef TCPWORKER_HPP
#define TCPWORKER_HPP

#include <QObject>
#include <QTcpSocket>

class TcpWorker : public QObject
{
   Q_OBJECT
public:
   explicit TcpWorker( QObject *parent = 0 );
   virtual ~TcpWorker();

signals:
   void received( const QByteArray &data );
   void connected();
   void disconnected();
   void stateChanged(QAbstractSocket::SocketState socketState);

public slots:
   void connectToHost( const QString &hostname, int port );
   void close();
   void send( const QByteArray &data );
   void readData();

private:
   QTcpSocket     *mpTcpSocket;
};

#endif // TCPWORKER_HPP
