#ifndef DHT11_LIB_H
#define DHT11_LIB_H

#define DEFAULT_DATA_PIN 2
#define SAMPLE_DELAY 1000    // SAMPLE DELAY SHOULD BE NO LESS THAN ONE SECOND!
#define STARTUP_DELAY 1000  // Startup delay should at least one second

#define __FORCE_INLINE__ __attribute__((always_inline))
#define CTOFAHR(n) ((typeof(n))((n)*9/5 + 32))
#define CTOKELV(n) ((typeof(n))((n) + (typeof(n))273.15))

enum DHT11_ERRORS {
  DHT11_SUCCESS = 0,
  DHT11_ERR_TIMEOUT,
  DHT11_ERR_CHECKSUM,
  DHT11_ERR_NOT_READY,
};

struct DHT11_WORD {
  union {
    struct {
      struct {
        uint8_t high;
        uint8_t low;
      } humid;
      struct {
        uint8_t high;
        uint8_t low: 7;
        uint8_t sign: 1;
      } temp;
      uint8_t sum;
    };
    uint8_t raw[5];
  };

  // methods
  uint8_t checksum();
  //void swapendian();
};

class DHT11_DEVICE {
  protected:
    // data
    uint8_t dpin;
    float temp;
    float humid;
    //uint16_t zerotemp;
    //uint16_t zerohumid;
    unsigned long last_sample;
    DHT11_WORD word;

    // methods
    int8_t sendstart();
    int8_t updateword();
    void graphsig(); // for debugging
    void formatword();

  public:
    // methods
    DHT11_DEVICE(uint8_t dpin);
    __FORCE_INLINE__ DHT11_DEVICE();
    // apparently these aren't needed
    //__FORCE_INLINE__ int8_t calibrate_temp(int16_t zero);
    //__FORCE_INLINE__ int8_t calibrate_humid(int16_t zero);
    float readtemp();
    float readhumid();
    static const char* geterr(int8_t err);
    int8_t isready();
    void debug();
};

#endif
