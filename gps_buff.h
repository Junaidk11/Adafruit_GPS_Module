/*
 * gps_buff.h
 *
 *  Created on: Aug 23, 2019
 *      Author: junaidkhan
 */

#ifndef GPS_BUFF_H_
#define GPS_BUFF_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * \brief           Buffer structure
 */
typedef struct {
    uint8_t* buff;                              /*!< Pointer to buffer data.
                                                    Buffer is considered initialized when `buff != NULL` and `size > 0` */
    size_t size;                                /*!< Size of buffer data. Size of actual buffer is `1` byte less than value holds */
    size_t r;                                   /*!< Next read pointer. Buffer is considered empty when `r == w` and full when `w == r - 1` */
    size_t w;                                   /*!< Next write pointer. Buffer is considered empty when `r == w` and full when `w == r - 1` */
} gps_buff_t;

/* GPS Buffer Prototypes */

uint8_t     buff_init(gps_buff_t* buff, void* buffdata, size_t size);
void        buff_free(gps_buff_t* buff);
void        buff_reset(gps_buff_t* buff);

/* Read/Write functions */
size_t      buff_write(gps_buff_t* buff, const void* data, size_t btw);
size_t      buff_read(gps_buff_t* buff, void* data, size_t btr);
size_t      buff_peek(gps_buff_t* buff, size_t skip_count, void* data, size_t btp);

/* Buffer size information */
size_t      buff_get_free(gps_buff_t* buff);
size_t      buff_get_full(gps_buff_t* buff);




#endif /* GPS_BUFF_H_ */
