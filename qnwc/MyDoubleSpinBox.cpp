#include "MyDoubleSpinBox.hpp"

#include <QKeyEvent>

MyDoubleSpinBox::MyDoubleSpinBox( QWidget *parent )
: QDoubleSpinBox( parent )
{
}


void MyDoubleSpinBox::keyPressEvent( QKeyEvent *event )
{
   switch( event->key() )
   {
   case Qt::Key_Return:
   case Qt::Key_Enter:
      emit returnPressed();
   default:
      break;
   }
   QDoubleSpinBox::keyPressEvent( event );
}
