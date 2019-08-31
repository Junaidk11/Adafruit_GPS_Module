/*
 * gps_buff.c
 *
 *  Created on: Aug 23, 2019
 *      Author: junaidkhan
 */

#include "gps_buff.h"


#define BUF_IS_VALID(b)                 ((b) != NULL && (b)->buff != NULL && (b)->size > 0)
#define BUF_MIN(x, y)                   ((x) < (y) ? (x) : (y))

/**
 * \brief           Initialize buffer handle to default values with size and buffer data array
 * \param[in]       buff: Buffer handle
 * \param[in]       buffdata: Pointer to memory to use as buffer data
 * \param[in]       size: Size of `buffdata` in units of bytes
 *                  Maximum number of bytes buffer can hold is `size - 1`
 * \return          `1` on success, `0` otherwise
 */
uint8_t
buff_init(gps_buff_t* buff, void* buffdata, size_t size){
    if (buff == NULL || buffdata == NULL || size == 0) {
            return 0;
        }

        memset(buff, 0x00, sizeof(*buff));

        buff->size = size;
        buff->buff = buffdata;

        return 1;
}


/**
 * \brief           Get number of bytes in buffer available to write
 * \param[in]       buff: Buffer handle
 * \return          Number of free bytes in memory
 */
size_t
buff_get_free(gps_buff_t* buff){
        size_t size, w, r;

        if (!BUF_IS_VALID(buff)) {
            return 0;
        }

        /* Use temporary values in case they are changed during operations */
        w = buff->w;
        r = buff->r;
        if (w == r) {
            size = buff->size;
        } else if (r > w) {
            size = r - w;
        } else {
            size = buff->size - (w - r);
        }

        /* Buffer free size is always 1 less than actual size */
        return size - 1;
}

/**
 * \brief           Write data to buffer
 *                  Copies data from `data` array to buffer and marks buffer as full for maximum `count` number of bytes
 * \param[in]       buff: Buffer handle
 * \param[in]       data: Pointer to data to write into buffer
 * \param[in]       btw: Number of bytes to write
 * \return          Number of bytes written to buffer.
 *                  When returned value is less than `btw`, there was not enough memory available
 *                  to copy full data array
 */

size_t
buff_write(gps_buff_t* buff, const void* data, size_t btw){
        size_t tocopy, free;
        const uint8_t* d = data;

        if (!BUF_IS_VALID(buff) || btw == 0) {
            return 0;
        }

        /* Calculate maximum number of bytes available to write */
        free = buff_get_free(buff);
        btw = BUF_MIN(free, btw);
        if (btw == 0) {
            return 0;
        }

        /* Step 1: Write data to linear part of buffer */
        tocopy = BUF_MIN(buff->size - buff->w, btw);
        memcpy(&buff->buff[buff->w], d, tocopy);
        buff->w += tocopy;
        btw -= tocopy;

        /* Step 2: Write data to beginning of buffer (overflow part) */
        if (btw > 0) {
            memcpy(buff->buff, (void *)&d[tocopy], btw);
            buff->w = btw;
        }

        if (buff->w >= buff->size) {
            buff->w = 0;
        }
        return tocopy + btw;
}

/**
 * \brief           Get number of bytes in buffer available to read
 * \param[in]       buff: Buffer handle
 * \return          Number of bytes ready to be read
 */
size_t
buff_get_full(gps_buff_t* buff){
       size_t w, r, size;

       if (!BUF_IS_VALID(buff)) {
           return 0;
       }

       /* Use temporary values in case they are changed during operations */
       w = buff->w;
       r = buff->r;
       if (w == r) {
           size = 0;
       } else if (w > r) {
           size = w - r;
       } else {
           size = buff->size - (r - w);
       }
       return size;
}


/**
 * \brief           Read data from buffer
 *                  Copies data from buffer to `data` array and marks buffer as free for maximum `btr` number of bytes
 * \param[in]       buff: Buffer handle
 * \param[out]      data: Pointer to output memory to copy buffer data to
 * \param[in]       btr: Number of bytes to read
 * \return          Number of bytes read and copied to data array
 */
size_t
buff_read(gps_buff_t* buff, void* data, size_t btr){
       size_t tocopy, full;
       uint8_t *d = data;

       if (!BUF_IS_VALID(buff) || btr == 0) {
           return 0;
       }

       /* Calculate maximum number of bytes available to read */
       full = buff_get_full(buff);
       btr = BUF_MIN(full, btr);
       if (btr == 0) {
           return 0;
       }

       /* Step 1: Read data from linear part of buffer */
       tocopy = BUF_MIN(buff->size - buff->r, btr);
       memcpy(d, &buff->buff[buff->r], tocopy);
       buff->r += tocopy;
       btr -= tocopy;

       /* Step 2: Read data from beginning of buffer (overflow part) */
       if (btr > 0) {
           memcpy(&d[tocopy], buff->buff, btr);
           buff->r = btr;
       }

       /* Step 3: Check end of buffer */
       if (buff->r >= buff->size) {
           buff->r = 0;
       }
       return tocopy + btr;
}
