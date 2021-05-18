#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdbool.h>

/**
 * @brief Initialize functionality of button
 *
 */
void btn_init(void);

/**
 * @brief Set state of button
 *
 * @param state New state
 */
void btn_set_state(bool state);

/**
 * @brief Get state of button
 *
 * @return true  Button is pressed
 * @return false Button is not pressed
 */
bool btn_get_state(void);

/**
 * @brief Software removing of button jitter
 *
 */
void btn_delete_jitter(void);

#endif /*BUTTON_H_*/
