#ifndef ANGLEDRAWWIDGET_HPP
#define ANGLEDRAWWIDGET_HPP

#include <QWidget>
#include <QPainter>
#include <QDebug>
#include <math.h>

#define PI 3.14159265

class AngleDrawWidget : public QWidget
{
   Q_OBJECT
public:
   explicit AngleDrawWidget( QWidget *parent = 0 );

signals:

public slots:
   void setAngle( double newAngle );

protected:
   void paintEvent( QPaintEvent *event );
   QSize sizeHint() const;

private:
   double mAngle;
   QPoint calculateAngle();

};

#endif // ANGLEDRAWWIDGET_HPP
