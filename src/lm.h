#ifndef LIGHT_MUSIC_H_
#define LIGHT_MUSIC_H_

typedef enum
{
    LM_LED_FUNC_SMOOTH = 0,
    LM_LED_FUNC_SHARP,
    LM_LED_FUNC_ENUM_SIZE
} lm_led_func_e;

/**
 * @brief Initialize functionality of light music
 *
 */
void lm_init(void);

/**
 * @brief Enable functionality of light music
 *
 */
void lm_enable(void);

/**
 * @brief Execute main processing of light music
 *
 * Call in infinite loop
 *
 */
void lm_process(void);

#endif /*LIGHT_MUSIC_H_*/
