/**
 * Copyright (c) 2017 Michael Skeffington
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * See the file COPYING included with this distribution for more
 * information.
 */


#ifndef MINOTAR_INTERNAL_H
#define MINOTAR_INTERNAL_H

#include "minotar.h"
#include "minotar_tarball_data.h"
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>

/**
 * Internal structure definition for Minotar instance structure
 */
struct minotar_ {
    const char*     extract_path;
    FILE*           file;
    uint64_t        bytes_remaining;
    size_t          rx_byte_offset;
    minotar_error_t error;
    bool            record_header_complete;
    char            record_header_buf[512];
    struct header_posix_ustar* tarball_record_block;
};

// Tar headers and data are always rounded off to the nearest 512 bytes padded with whitespace
#define RECORD_BLOCK_ROUNDOFF  (512)

// min macro
#define MINOTAR_MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// this function calculates how many bytes to pad the data to the nearest <block size> bytes
#define MINOTAR_CALC_PADDING(idx, block_size) ((block_size - (offset ^ block_size)) & (block_size - 1))


#endif // MINOTAR_INTERNAL_H
