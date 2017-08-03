class Clock4
{
  public:
    uint8_t value();
    uint8_t base();
    boolean expired();
    void period(uint16_t _period);
    void rate(uint16_t _rate);
    void attachInterrupt(void (*_callback)(void));

  private:
    uint8_t m_base;
    void init(uint32_t cycles);
};


