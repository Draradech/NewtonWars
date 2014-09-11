#ifndef MYDOUBLESPINBOX_HPP
#define MYDOUBLESPINBOX_HPP

#include <QDoubleSpinBox>

class MyDoubleSpinBox : public QDoubleSpinBox
{
Q_OBJECT
public:
   explicit MyDoubleSpinBox( QWidget *parent = 0 );

signals:
   void returnPressed();

protected:
   void keyPressEvent( QKeyEvent *event );
};



#endif // MYDOUBLESPINBOX_HPP
