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
#include "zlib.h"
#include <unistd.h>
#include <stdio.h>

static void print_gzip_error(int retcode)
{
    switch (retcode) {
        case Z_NEED_DICT:
            printf("Zlib dictionary Error.\n");
        case Z_DATA_ERROR:
            printf("Zlib invalid data error.\n");
            break;
        case Z_MEM_ERROR:
            printf("Zlib out of memory.\n");
            break;
        case Z_BUF_ERROR:
            printf("Zlib invalid buffer.\n");
            break;
        default:
            printf("Zlib unknown error: %d\n", retcode);
            break;
    }
}

int main(int argc, char* argv[])
{
    FILE* some_archive = NULL;
    z_stream z_context;
    minotar_t* minotar_context;
    char* file_name = NULL;
    char file_buf[256] = {0};
    unsigned char ex_file_buf[256] = {0};
    size_t read_size = 0;
    char target_dir[] = "./";
    int z_ret = Z_OK;
    
    if(argc < 2) {
        printf("Usage: minotar <filename>.tar\n");
        goto exit;
    }
    file_name = argv[1];
    
    if(access(file_name, F_OK) < 0) {
        printf("file <%s> does not exist. Cannot parse.\n", file_name);
        goto exit;
    }
    
    // initialize zlib
    z_context.zalloc = Z_NULL;
    z_context.zfree = Z_NULL;
    z_context.opaque = Z_NULL;
    z_context.avail_in = 0;
    z_context.next_in = Z_NULL;

    // this weird init is how you tell zlib you want to decode gzip
    if(inflateInit2(&z_context, 16 + MAX_WBITS) != Z_OK) {
        printf("Zlib failed to initialize.\n");
        goto exit;
    }
    
    // initialize the library
    if(minotar_init(&minotar_context) != MINOTAR_noerror) {
        printf("Minotar failed to initialize.\n");
        goto exit;
    }
    
    // tell minotar to decode to the local directory
    if(minotar_set_extract_directory(minotar_context, target_dir) != MINOTAR_noerror) {
        printf("Minotar set directory.\n");
        goto exit;
    }
    
    // open up the file for reading
    some_archive = fopen(file_name, "rb");
    if(some_archive == NULL) {
        printf("archive <%s> failed to open.\n", file_name);
        goto exit;
    }
    
    
    // read the file in in 256 byte chunks and decode it.
    while((read_size = fread(file_buf, 1, sizeof(file_buf), some_archive)) > 0) {
        z_context.avail_in = read_size;
        if (z_context.avail_in == 0) {
            continue;
        }

        // zlib requires unsigned... annoying
        z_context.next_in = (unsigned char*)file_buf;

        // our output buffer is small so just loop till we copy out all of it
        do {
            z_context.avail_out = sizeof(ex_file_buf);
            z_context.next_out = ex_file_buf;
            
            if((z_ret = inflate(&z_context, Z_NO_FLUSH)) < 0 && z_ret != Z_BUF_ERROR) {
                print_gzip_error(z_ret);
                goto exit;
            }

            size_t decode_size = sizeof(ex_file_buf) - z_context.avail_out;

            // pass the decoded bytes to minotar
            if(minotar_decode(minotar_context, (char*) ex_file_buf, decode_size) != MINOTAR_noerror) {
                printf("decode failed.  exiting.\n");
                goto exit;
            }
        } while (z_context.avail_out == 0);

        // done when inflate() says it's done
        if(z_ret == Z_STREAM_END) {
            // clean up the stream.
            inflateEnd(&z_context);
            z_ret = Z_OK;
        }
    }
    
    printf("successfully decoded %s.  exiting.\n", file_name);
    
exit:
    // tear down the library
    if(minotar_context != NULL)
        minotar_deinit(&minotar_context);
    
    // clean up zlib.
    inflateEnd(&z_context);
    
    return 0;
}
