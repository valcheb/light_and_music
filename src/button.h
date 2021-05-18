#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdbool.h>

void btn_init(void);
void btn_set_state(bool state);
bool btn_get_state(void);
void btn_delete_jitter(void);

#endif /*BUTTON_H_*/
