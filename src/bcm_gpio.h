#ifndef BCM2835GPIO_H_
#define BCM2835GPIO_H_

typedef enum {
    FSEL_IN = 0,
    FSEL_OUT = 1,
    FSEL_ALT0 = 4,
    FSEL_ALT1 = 5,
    FSEL_ALT2 = 6,
    FSEL_ALT3 = 7,
    FSEL_ALT4 = 3,
    FSEL_ALT5 = 2 
} GPIOFunction;

typedef enum {
    PUD_OFF = 0,
    PUD_DOWN = 1,
    PUD_UP = 2,
} GPIOPUDControl;

int gpio_access();
void gpio_deaccess();
void gpio_confpin(int n, GPIOFunction fsel, GPIOPUDControl pud);
int gpio_readpin(int n);
void gpio_writepin(int n, int v);

#endif /* BCM2835GPIO_H_ */