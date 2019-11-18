#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#include "bcm_pwm.h"
 
#define CM_PWM_PASSWD (0x5a << 24)

#define CM_BASE_ADDR (0x20101000)
#define CM_PWMCTL_OF (0xA0)
#define CM_PWMDIV_OF (0xA4)
#define CM_BLOCK_SIZE (0x1C8)

#define CM_PWMCTL_SRC_OF (0)
#define CM_PWMCTL_SRC_OSC (1)
#define CM_PWMCTL_ENAB_OF (4)
#define CM_PWMCTL_BUSY_OF (7)
#define CM_PWMCTL_MASH_OF (9)
#define CM_PWMDIV_DIVF_OF (0)
#define CM_PWMDIV_DIVI_OF (12)

#define PWM_BASE_ADDR (0x2020C000)
#define PWM_CTL_OF (0x0)
#define PWM_STA_OF (0x4)
#define PWM_RNG1_OF (0x10)
#define PWM_DAT1_OF (0x14)
#define PWM_RNG2_OF (0x20)
#define PWM_DAT2_OF (0x24)
#define PWM_BLOCK_SIZE (0x28)

#define PWM_CTL_PWEN1_OF (0)
#define PWM_CTL_PWEN2_OF (8)
#define PWM_CTL_MSEN1_OF (7)
#define PWM_CTL_MSEN2_OF (15)

volatile uint8_t* cm_base = NULL;
volatile uint8_t* pwm_base = NULL;

#define PWM_CTL (*((volatile uint32_t*)((pwm_base) + PWM_CTL_OF)))
#define PWM_DAT1 (*((volatile uint32_t*)((pwm_base) + PWM_DAT1_OF)))
#define PWM_DAT2 (*((volatile uint32_t*)((pwm_base) + PWM_DAT2_OF)))
#define PWM_RNG1 (*((volatile uint32_t*)((pwm_base) + PWM_RNG1_OF)))
#define PWM_RNG2 (*((volatile uint32_t*)((pwm_base) + PWM_RNG2_OF)))

#define CM_PWMCTL (*((volatile uint32_t*)(cm_base + CM_PWMCTL_OF)))
#define CM_PWMDIV (*((volatile uint32_t*)(cm_base + CM_PWMDIV_OF)))

int pwm_init(int ch1_en, int ch2_en) {
    int devmem_fd = open("/dev/mem", O_RDWR|O_SYNC); 
    if(devmem_fd < 0) {
        return -1;
    }
    pwm_base = mmap(NULL, PWM_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, PWM_BASE_ADDR);
    cm_base = mmap(NULL, CM_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem_fd, CM_BASE_ADDR);
    if(cm_base == MAP_FAILED || pwm_base == MAP_FAILED) {
        return -1;
    }    
    close(devmem_fd);

    //disable pwm clock
    CM_PWMCTL = (CM_PWMCTL & ~(1 << CM_PWMCTL_ENAB_OF)) |  CM_PWM_PASSWD;
    while((CM_PWMCTL & (1 << CM_PWMCTL_BUSY_OF)) != 0); 

    //integer clock div 2    ------------ min 2 !!
    CM_PWMDIV = (2 << CM_PWMDIV_DIVI_OF) | CM_PWM_PASSWD;
    
    //setup source clock
    CM_PWMCTL = CM_PWMCTL_SRC_OSC | (1 << CM_PWMCTL_MASH_OF) | CM_PWM_PASSWD;
    //en clock
    CM_PWMCTL |= ((1 << CM_PWMCTL_ENAB_OF) | CM_PWM_PASSWD);
    while((CM_PWMCTL & (1 << CM_PWMCTL_BUSY_OF)) == 0); 

    //config pwm module
    
    PWM_CTL = 0;
    if(ch1_en) {
        PWM_CTL |= ((1 << PWM_CTL_PWEN1_OF) | (1 << PWM_CTL_MSEN1_OF));
    }
    if(ch2_en) {
        PWM_CTL |= ((1 << PWM_CTL_PWEN2_OF) | (1 << PWM_CTL_MSEN2_OF));
    }
/*
    puts("dump pwm regs:");
    printf("PWM_CTL=%d, PWM_DAT1=%d, PWM_RNG1=%d, PWM_DAT2=%d, PWM_RNG2=%d\n", PWM_CTL, PWM_DAT1, PWM_RNG1, PWM_DAT2, PWM_RNG2);
    puts("dump cm regs:");
    printf("CM_PWMCTL=%d, CM_PWMDIV=%d\n", CM_PWMCTL, CM_PWMDIV);
*/
    return 0;
}

void pwm_set_ch1(uint32_t period, uint32_t width) {
    PWM_DAT1 = width;
    PWM_RNG1 = period;
}

void pwm_set_ch2(uint32_t period, uint32_t width) {
    PWM_DAT2 = width;
    PWM_RNG2 = period;
}

