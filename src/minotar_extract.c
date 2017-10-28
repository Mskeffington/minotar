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

#include "minotar.h"
#include "minotar_internal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


// ---------------- FORWARD DECLARATIONS ----------------------

static bool minotar_header_verify_checksum(minotar_t* instance);
static size_t minotar_header_get_path_length(minotar_t* instance);
static void minotar_header_parse_path(minotar_t* instance, char* filename, size_t fnamelen);
static bool minotar_create_file(minotar_t* instance);
static bool minotar_parse_record_block(minotar_t* instance);
static size_t minotar_parse(minotar_t* instance, const char* bytes, size_t length);


// ------------------ INLINE FUNCTIONS ------------------------

/**
 * @return This function returns the file size of the current file
 */
static inline uint64_t minotar_get_file_size(minotar_t* instance)
{
    // File size is 6 bytes of ascii representing octal
    // followed by one byte of ' ' and ending with '\0'
    return strtoul(instance->tarball_record_block->size, NULL, 8);
}

/**
 * @return This function returns the file mode of the current file
 */
static inline mode_t minotar_get_file_mode(minotar_t* instance)
{
    // File size is 7 bytes of ascii representing octal with 1 byte NULL termination
    return (mode_t) strtoul(instance->tarball_record_block->mode, NULL, 8);
}

/**
 * @return This function returns the file user id of the current file
 */
static inline uid_t minotar_get_file_uid(minotar_t* instance)
{
    // File size is 7 bytes of ascii representing octal with 1 byte NULL termination
    return (uid_t) strtoul(instance->tarball_record_block->uid, NULL, 8);
}

/**
 * @return This function returns the file group id of the current file
 */
static inline uid_t minotar_get_file_gid(minotar_t* instance)
{
    // File size is 7 bytes of ascii representing octal with 1 byte NULL termination
    return (gid_t) strtoul(instance->tarball_record_block->gid, NULL, 8);
}

/**
 * Helper function to check for the ustar keyword indicating extended path length.
 * @return This function returns true when the "ustar" keyword is present.
 */
static inline bool minotar_header_has_extended_path(minotar_t* instance)
{
    // new IEEE spec for universal tarball standard places the keyword
    // ascii "ustar\0" after the original header
    return !memcmp(instance->tarball_record_block->magic, "ustar\0", 6);
}

/**
 * @return this function returns the version number for block special and char special files.
 */
static inline dev_t minotar_header_get_device_version(minotar_t* instance)
{
    if(instance == NULL) return 0;
    uint32_t major = strtoul(instance->tarball_record_block->devmajor, NULL, 8);
    uint32_t minor = strtoul(instance->tarball_record_block->devminor, NULL, 8);
    return (dev_t) major << 32 & minor;
}


// ------------------ PUBLIC FUNCTIONS ------------------------

/**
 * Initialize the Minotar library.  This function allocates 560 bytes of data for
 * the interal structure.
 * 
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_init(minotar_t** p_instance)
{
    if(p_instance == NULL)
        return MINOTAR_invalid_parameter;
    
    *p_instance = (minotar_t*) malloc(sizeof(minotar_t));
    
    if(*p_instance == NULL)
        return MINOTAR_out_of_memory;
        
    
    (*p_instance)->tarball_record_block = (struct header_posix_ustar*) (*p_instance)->record_header_buf;
    
    return MINOTAR_noerror;
}

/**
 * Deinitialize the Minotar library and clean up allocated memory.
 * 
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_deinit(minotar_t** p_instance)
{
    if(p_instance == NULL || *p_instance == NULL)
        return MINOTAR_invalid_parameter;
    
    if((*p_instance)->file != NULL)
        fclose((*p_instance)->file);
    
    free(*p_instance);
    *p_instance = NULL;
    
    return MINOTAR_noerror;
}

/**
 * Instruct Minotar to extract this archive to a path other than ./
 * This path may be fully qualified or it can be relative.
 * 
 * @param path  A null-terminated pointer to a path on the filesystem.
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_set_extract_directory(minotar_t* instance, const char* path)
{
    struct stat st = {0};
    
    if(instance == NULL || path == NULL)
        return MINOTAR_invalid_parameter;
    
    if(instance->rx_byte_offset != 0)
        return MINOTAR_set_path_before_decode;
        
    
    // if the path directory doesnt exist, create it.
    if(stat(path, &st) == -1)
        return MINOTAR_invalid_path;
        
    instance->extract_path = path;
    
    return MINOTAR_noerror;
}

/**
 * @brief Reset this instance of minotar.  
 * A reset clears all errors and expects the beginning of a record block as its first
 * input.  If a reset is performed in the middle of decoding, unexpected results can occur.
 * 
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_reset(minotar_t* instance)
{
    if(instance == NULL)
        return MINOTAR_invalid_parameter;
    
    instance->bytes_remaining = 0;
    instance->rx_byte_offset = 0;
    instance->error = MINOTAR_noerror;
    instance->record_header_complete = false;
    if(instance->file != NULL)
        fclose(instance->file);
    
    instance->file = NULL;
    
    // clear the data in the header
    memset(instance->record_header_buf, 0, sizeof(instance->record_header_buf));
    return MINOTAR_noerror;
}

/**
 * Decode the next block of data.  This function automatically writes the file to disk.
 * 
 * @param bytes     A buffer of bytes as it comes in from the file.
 * @param length    the length of buffer bytes.
 * @return an error code as defined in the error struct.
 */
minotar_error_t minotar_decode(minotar_t* instance, const char* bytes, size_t length)
{
    size_t parsed = 0;
    
    if(instance == NULL || bytes == NULL)
        return MINOTAR_invalid_parameter;
    
    // Recursively call the parse function to work our way through all the data.
    // Of course, this is meant for embedded so lets use loop based recursion.
    while(instance->error == MINOTAR_noerror && parsed < length) {
        parsed += minotar_parse(instance, &bytes[parsed], length - parsed);
        if(parsed > length) {
            instance->error = MINOTAR_unknown_error;
        }
    }
    
    return instance->error;
}



// ------------------ PRIVATE FUNCTIONS ------------------------

/**
 * 
 * @return This function returns the full path length.
 */
static size_t minotar_header_get_path_length(minotar_t* instance)
{
    const size_t max_prefix_length = sizeof(instance->tarball_record_block->prefix);
    const size_t max_name_length = sizeof(instance->tarball_record_block->name);
    size_t length = 0;
    
    if(instance->extract_path != NULL) {
        length += strlen(instance->extract_path);
        length += 1; // add 1 for added '/'
    }
    
    if(minotar_header_has_extended_path(instance)) {
        length += MINOTAR_MIN(strlen(instance->tarball_record_block->prefix), max_prefix_length);
        length += 1; // add 1 for added '/'
    }
    
    length += MINOTAR_MIN(strlen(instance->tarball_record_block->name), max_name_length);
    
    return length;
}

/**
 * @brief Verify the header checksum of the tarball.
 * weird quirk... the header checksum isnt defined as signed or unsigned so different
 * implemetations use whatever they feel like.  So we need to test both.
 * 
 * @return This function returns the boolean validity of the checksum.
 */
static bool minotar_header_verify_checksum(minotar_t* instance)
{
    bool result = false;
       
    // Checksum is 6 bytes of octal one byte of ' ' and ending with '\0'
    uint32_t rx_checksum = strtoul(instance->tarball_record_block->checksum, NULL, 8);
    int32_t rx_schecksum = (int32_t) rx_checksum;
    uint32_t calc_checksum = 0;
    int32_t calc_schecksum = 0;

    // the the checksum assumes that the 8 bytes allocated
    // for checksum are assigned ascii ' ' or 0x30
    const size_t checksum_length = sizeof(instance->tarball_record_block->checksum);
    memset(instance->tarball_record_block->checksum, ' ', checksum_length);

    // add all the signed and unsigned bytes of the header simultaniously
    for(size_t idx = 0; idx < sizeof(instance->record_header_buf); ++idx) {
        calc_checksum += (uint8_t) instance->record_header_buf[idx];
        calc_schecksum += instance->record_header_buf[idx];
    }

    // if either of the checksums match, we have a valid checksum
    if(rx_checksum == calc_checksum || rx_schecksum == calc_schecksum)
        result = true;
    
    return result;
}

/**
 * This function appends the relative path + file name.
 * 
 * @param filename  a pointer to a char buffer into which we will write the file name
 * @param fnamelen  the size of the filename buffer
 */
static void minotar_header_parse_path(minotar_t* instance, char* filename, size_t fnamelen)
{
    // prepend the root directory if we have one
    if(instance->extract_path != NULL) {
        // assign the file path the the name
        int extract_dir_size = snprintf(filename, fnamelen, "%s/", instance->extract_path);
        filename += extract_dir_size - 1; // subtract 1 to eliminate null byte
        fnamelen -= extract_dir_size - 1;
    }
    
    // extended header path prefix gets added first follwed by a /
    if(minotar_header_has_extended_path(instance)) {
        // assign the file path the the name
        int prefix_size = snprintf(filename, fnamelen, "%s/", instance->tarball_record_block->prefix);
        filename += prefix_size - 1; // subtract 1 to eliminate null byte
        fnamelen -= prefix_size - 1;
    }

    // append the filename
    snprintf(filename, fnamelen, "%s", instance->tarball_record_block->name);
}

/**
 * Create the next file in the tarball.
 * 
 * @return This function returns whether the file was successfully created.
 */
static bool minotar_create_file(minotar_t* instance)
{
    int result = 0;
    mode_t mode = {0};
    dev_t ver = {0};
    char* path = NULL;
    size_t path_length = 0;
    // gid_t groupid = {0};
    // uid_t userid = {0};
    
    if(instance == NULL) return false;
    
    path_length = minotar_header_get_path_length(instance);
    path = (char*) malloc(path_length);
    if(path == NULL) {
        instance->error = MINOTAR_out_of_memory;
        return false;
    }
    
    minotar_header_parse_path(instance, path, path_length);
    
    // get the rest of the file info
    ver = minotar_header_get_device_version(instance);
    mode = minotar_get_file_mode(instance);
    // groupid = minotar_get_file_gid(instance);
    // userid = minotar_get_file_uid(instance);
    
    
    switch(instance->tarball_record_block->typeflag) {
        case FILE_TYPE_hard_link:
            result = link(instance->tarball_record_block->linkname, path);
            break;
        case FILE_TYPE_symlink:
#if defined (symlink)
            result = symlink(instance->tarball_record_block->linkname, path);
#elif defined (mknod)
            mode |= S_IFLNK;
            result = mknod(path, mode, 0);
            result = link(instance->tarball_record_block->linkname, path);
#endif
            break;
        case FILE_TYPE_char_special:
#if defined (mknod)
            mode |= S_IFCHR;
            result = mknod(path, mode, ver);
#endif
            break;
        case FILE_TYPE_block_special:
#if defined (mknod)
            mode |=  S_IFBLK;
            result = mknod(path, mode, ver);
#endif
            break;
        case FILE_TYPE_directory:
            result = mkdir(path, mode);
            //mode |= S_IFDIR;
            //result = mknod(path, mode, 0);
            break;
        case FILE_TYPE_fifo:
            result = mkfifo(path, mode);
            //mode |= S_IFIFO;
            //result = mknod(path, mode, 0);
            break;
        case FILE_TYPE_normal_file:
        case FILE_TYPE_normal_file_alt:
        // This continuous file is unsupported in most UNIX systems so handle it
        // like a normal file.
        case FILE_TYPE_continuous_file:
        // Handle all unknown types as normal files to maintain POSIX compliance.
        default:
#if defined (mknod)
            mode |= S_IFREG;
            result = mknod(path, mode, 0);
#else
            (void) ver;
#endif
            instance->file = fopen(path, "wb");
            break;
    }
    
    // it seems like a poor choice to set the ownership
    // chown(path, userid, groupid);
    chmod(path, mode);
    
    if(path != NULL)
        free(path);
    
    return result == 0;
}

/**
 * Parse a completed record block header.
 * 
 * @return This function returns whether the header block parsing was successful.
 */
static bool minotar_parse_record_block(minotar_t* instance)
{
    // verify tarball header checksum.
    if(!minotar_header_verify_checksum(instance)) {
        instance->error = MINOTAR_invalid_checksum;
        return false;
    }
    
    instance->record_header_complete = true;

    // the instance->tarball_record_block doesnt count in the filesize so reset it
    instance->rx_byte_offset = 0;
    
    if(!minotar_create_file(instance)) {
        instance->error = MINOTAR_failed_to_create_file;
        return false;
    }
    
    instance->bytes_remaining = minotar_get_file_size(instance);
    
    return true;
}

/**
 * Go through as many bytes as we can and write them out.  Any remaining bytes are returned
 * to the parent so the parsing may continue.
 * 
 * @return This function returns the number of bytes parsed.
 */
static size_t minotar_parse(minotar_t* instance, const char* bytes, size_t length)
{
    size_t offset = 0;
    
    // Check to see if we have gotten the whole record block header
    if(!instance->record_header_complete) {
        ssize_t header_write_size = MINOTAR_MIN(RECORD_BLOCK_ROUNDOFF - instance->rx_byte_offset, length);
        if(header_write_size) {
            // copy as much as we can to the instance header buffer
            memcpy(&instance->record_header_buf[instance->rx_byte_offset], &bytes[offset], header_write_size);

            instance->rx_byte_offset += header_write_size;
            offset += header_write_size;

            if(instance->rx_byte_offset == RECORD_BLOCK_ROUNDOFF) {
                minotar_parse_record_block(instance);
            }
        }
    }
    
    // write the next set of bytes to current file
    ssize_t write_size = MINOTAR_MIN(length - offset, instance->bytes_remaining);
    if(write_size > 0) {
        if(instance->file != NULL)
            fwrite(&bytes[offset], sizeof(char), write_size, instance->file);

        // increment our position
        offset += write_size;
        instance->rx_byte_offset += write_size;
        instance->bytes_remaining -= write_size;
    }

    // If we have completed writing the file and we still have bytes, add up to 511 bytes for padding
    if(instance->record_header_complete && instance->bytes_remaining == 0) {
        // calculate how many bytes we need to pad to 512 bytes
        size_t padding_remaining = MINOTAR_CALC_PADDING(instance->rx_byte_offset, RECORD_BLOCK_ROUNDOFF);
        size_t padding_size = MINOTAR_MIN(padding_remaining, length - offset);
        
        instance->rx_byte_offset += padding_size;
        offset += padding_size;
        
        if((instance->rx_byte_offset & 512) == 0)
        minotar_reset(instance);
    }
    
    return offset;
}
