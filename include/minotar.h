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


#ifndef MINOTAR_TARBALL_EXTRACT_H
#define MINOTAR_TARBALL_EXTRACT_H

#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>

/**
 * Minotar is a MINimal memory Overhead TARball extraction library.  It accomplishes
 * this by only holding 560 bytes at a time in memory and parsing the incoming data as a
 * stream.  Each file is parsed and written to disk in-band.  A system which has little
 * memory can effectively receive a file from an external source without needing enough
 * room to store both the packaged tarball on disk or in memory at the same time as the 
 * divided file data.  This mechanism is very useful for things such as firmware updates
 * or live file-based data streams.
 * 
 * It is very simple to wrap Minotar and give gzip functionality.  See the examples 
 * directory for usage.
 */


/**
 * Minotar error codes.
 */
typedef enum minotar_error_ {
    MINOTAR_noerror = 0,
    MINOTAR_invalid_parameter,
    MINOTAR_set_path_before_decode,
    MINOTAR_invalid_checksum,
    MINOTAR_invalid_path,
    MINOTAR_failed_to_create_file,
    MINOTAR_header_invalid,
    MINOTAR_out_of_memory,
    MINOTAR_unknown_error
} minotar_error_t;


// miniature memory footprint C tar stream de-archiver.
typedef struct minotar_ minotar_t;

/**
 * Initialize the Minotar library.  This function allocates 560 bytes of data for
 * the interal structure.
 * 
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_init(minotar_t** p_instance);

/**
 * Deinitialize the Minotar library and clean up allocated memory.
 * 
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_deinit(minotar_t** p_instance);


/**
 * @brief Reset this instance of minotar.  
 * A reset clears all errors and expects the beginning of a record block as its first
 * input.  If a reset is performed in the middle of decoding, unexpected results can occur.
 * 
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_reset(minotar_t* instance);

/**
 * Instruct Minotar to extract this archive to a path other than ./
 * This path may be fully qualified or it can be relative.
 * 
 * @param path  A null-terminated pointer to a path on the filesystem.
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_set_extract_directory(minotar_t* instance, const char* path);


/**
 * Decode the next block of data.  This function automatically writes the file to disk.
 * 
 * @param bytes     A buffer of bytes as it comes in from the file.
 * @param length    the length of buffer bytes.
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_decode(minotar_t* instance, const char* bytes, size_t length);

#endif // MINOTAR_TARBALL_EXTRACT_H

