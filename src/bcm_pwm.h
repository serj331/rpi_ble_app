#ifndef BCM_PWM_H_
#define BCM_PWM_H_
int pwm_init(int ch1_en, int ch2_en);
void pwm_set_ch1(uint32_t period, uint32_t width);
void pwm_set_ch2(uint32_t period, uint32_t width);
#endif

