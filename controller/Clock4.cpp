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
  uint8_t psk = 1;
  
  while(cycles > 255)
  {
    cycles /= 2;
    psk += 1;
  }
  
  if (psk > 15)
  {
    psk = 15;
    cycles = 255;
  }

  m_base = cycles;

  TCCR4A = 0;
  TCCR4B = (1 << PSR4) | psk;
  TCCR4C = 0;
  TCCR4D = 0;
  TCCR4E = 0;
  OCR4C = m_base - 1;
  TCNT4 = 0;
  TIFR4 = (1 << TOV4);
  TIMSK4 = (1 << TOIE4);
}

uint8_t Clock4::value()
{
  return TCNT4;
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

ISR(TIMER4_OVF_vect)
{
  clock4_trigger = 1;
  if(callback) callback();
}
