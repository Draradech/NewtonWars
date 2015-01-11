#include "AngleDrawWidget.hpp"

AngleDrawWidget::AngleDrawWidget(QWidget *parent)
: QWidget(parent)
{
}


void AngleDrawWidget::paintEvent( QPaintEvent */*event*/ )
{
   QPoint l_targetPoint = calculateAngle();
   QPoint l_center(this->width() / 2, this->height() / 2);
   QPainter l_painter(this);
   l_painter.setRenderHint(QPainter::Antialiasing, true);
   l_painter.setPen(QColor(0,0,0));
   for( int i = 0; i < 4; ++i)
   {
      l_painter.drawEllipse( l_center, i, i );
   }
   l_painter.drawLine(l_center, l_targetPoint);
}

QSize AngleDrawWidget::sizeHint() const
{
   int size = height() > width() ? height() : width();
   if( size < 50 )
   {
      size = 50;
   }
   return QSize( size, size );
}


QPoint AngleDrawWidget::calculateAngle()
{
   int l_hcenter = this->width() / 2;
   int l_vcenter = this->height() / 2;
   int l_min = l_hcenter < l_vcenter ? l_hcenter : l_vcenter;

   double l_vertical = sin(mAngle * PI / 180) * -1.0;
   double l_horizontal = cos(mAngle * PI / 180);

   double l_tleft = (l_horizontal * l_min) + double(l_hcenter);
   double l_ttop = (l_vertical * l_min) + double(l_vcenter);
   return QPoint(l_tleft, l_ttop);
}


void AngleDrawWidget::setAngle( double newAngle )
{
   mAngle = newAngle;
   repaint();
}
