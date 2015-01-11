#include "TCPWorker.hpp"
#include <QTcpSocket>

#include <QtDebug>

TcpWorker::TcpWorker( QObject *parent )
: QObject( parent )
, mpTcpSocket( new QTcpSocket(this) )
{
   connect( mpTcpSocket, SIGNAL(connected()),
            this, SIGNAL(connected()) );
   connect( mpTcpSocket, SIGNAL(disconnected()),
            this, SIGNAL(disconnected()) );
   connect( mpTcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this, SIGNAL(stateChanged(QAbstractSocket::SocketState)) );
}


TcpWorker::~TcpWorker()
{
}


void TcpWorker::connectToHost( const QString &hostname, int port )
{
   mpTcpSocket->connectToHost( hostname, static_cast<quint16>(port) );
}


void TcpWorker::close()
{
   mpTcpSocket->close();
}


void TcpWorker::send( const QByteArray &data )
{
   if( mpTcpSocket->isOpen() )
   {
      mpTcpSocket->write( data );
      mpTcpSocket->flush();
   }
}


void TcpWorker::readData()
{
   emit received( mpTcpSocket->readAll() );
}
