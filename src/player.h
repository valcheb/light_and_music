#ifndef PLAYER_H_H
#define PLAYER_H_H

#include <stdint.h>

void player_init(void);
void player_enable(void);
void player_send(uint16_t *buffer, int size);

#endif /*PLAYER_H_*/
