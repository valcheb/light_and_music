#ifndef PLAYER_H_H
#define PLAYER_H_H

#include <stdint.h>

/**
 * @brief Initialize functionality of CL43L22 as player
 *
 */
void player_init(void);

/**
 * @brief Enable functionality of CL43L22
 *
 */
void player_enable(void);

/**
 * @brief Send sound data to CL43L22
 *
 * @param buffer Pointer to sound buffer
 * @param size   Size of sound buffer
 */
void player_send(uint16_t *buffer, int size);

#endif /*PLAYER_H_*/
