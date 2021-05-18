#ifndef LIGHT_MUSIC_H_
#define LIGHT_MUSIC_H_

typedef enum
{
    LM_LED_FUNC_SMOOTH = 0,
    LM_LED_FUNC_SHARP,
    LM_LED_FUNC_ENUM_SIZE
} lm_led_func_e;

void lm_init(void);
void lm_enable(void);
void lm_process(void);

#endif /*LIGHT_MUSIC_H_*/
