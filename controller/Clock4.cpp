#include <Arduino.h>
#include "Clock4.h"

static volatile uint8_t clock4_trigger;
static volatile void (*callback)(void);

void Clock4::period(uint16_t _period)
{
  init(F_CPU / 1000 * _period / 1000);
}

void Clock4::rate(uint16_t _rate)
{
  init(F_CPU / _rate);
}

void Clock4::attachInterrupt(void (*_callback)(void))
{
  callback = _callback;
}

void Clock4::init(uint32_t cycles)
{
  if(cycles > 65535)
  {
    cycles = 65535;
  }

  m_base = cycles;

  TCCR1A = (1 << WGM10);
  TCCR1B = (1 << WGM13) | (1 << CS10);
  TCCR1C = 0;
  OCR1A = m_base;
  TCNT1 = 0;
  TIFR1 = (1 << TOV1);
  TIMSK1 = (1 << TOIE1);
}

uint8_t Clock4::value()
{
  return TCNT3;
}

uint8_t Clock4::base()
{
  return m_base;
}

boolean Clock4::expired()
{
  boolean ret = false;
  
  if(clock4_trigger)
  {
    clock4_trigger = 0;
    ret = true;
  }
  
  return ret;
}

ISR(TIMER1_OVF_vect)
{
  clock4_trigger = 1;
  if(callback) callback();
}
