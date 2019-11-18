#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include "bcm_gpio.h"
 
#define GPIO_BASE_ADDR (0x20200000)
#define GPIO_BLOCK_SIZE (0xFF)

#define GPIO_GPSET0_OF (7)
#define GPIO_GPSET1_OF (8)
#define GPIO_GPCLR0_OF (10)
#define GPIO_GPCLR1_OF (11)
#define GPIO_GPLEV0_OF (13)
#define GPIO_GPLEV1_OF (14)
#define GPIO_GPPUD_OF (37)
#define GPIO_GPPUDCLK0_OF (38)
#define GPIO_GPPUDCLK1_OF (39)

volatile uint32_t* gpio_base = NULL;

int gpio_access() {
    int devmem_fd = open("/dev/mem", O_RDWR|O_SYNC); 
    if(devmem_fd < 0) {
        return -1;
    }
    gpio_base = mmap(NULL, GPIO_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, GPIO_BASE_ADDR);
    close(devmem_fd);
    if(gpio_base == MAP_FAILED) {
        return -1;
    }    
    return 0;
}

void gpio_deaccess() {
    munmap((void*)gpio_base, GPIO_BLOCK_SIZE);
}

void gpio_confpin(int n, GPIOFunction fsel, GPIOPUDControl pud) {
    int selx_pos = (n % 10) * 3;
    gpio_base[n / 10] = (gpio_base[n / 10] & ~(0x7 << selx_pos)) | (fsel << selx_pos);
    
    if(pud == PUD_DOWN || pud == PUD_UP) {
        *(gpio_base + GPIO_GPPUD_OF) = pud;
        usleep(1000);
        if(n > 31) {
            *(gpio_base + GPIO_GPPUDCLK1_OF) = 1 << (n - 32);
        } else {
            *(gpio_base + GPIO_GPPUDCLK0_OF) = 1 << n;
        }
        usleep(1000);
        *(gpio_base + GPIO_GPPUD_OF) = 0;
        *(gpio_base + GPIO_GPPUDCLK1_OF) = 0;
        *(gpio_base + GPIO_GPPUDCLK0_OF) = 0;
    }
}

int gpio_readpin(int n) {
    if(n > 31) {
        if(*(gpio_base + GPIO_GPLEV1_OF) & (1 << (n - 32))) {
            return 1;
        }
    } else {
        if(*(gpio_base + GPIO_GPLEV0_OF) & (1 << n)) {
            return 1;
        }
    }
    return 0;
}

void gpio_writepin(int n, int v) {
    if(v) {
        if(n > 31) {
            *(gpio_base + GPIO_GPSET1_OF) = 1 << (n - 32);
        } else {
            *(gpio_base + GPIO_GPSET0_OF) = 1 << n;
        }
    } else {
        if(n > 31) {
            *(gpio_base + GPIO_GPCLR1_OF) = 1 << (n - 32);
        } else {
            *(gpio_base + GPIO_GPCLR0_OF) = 1 << n;
        }
    }
}
