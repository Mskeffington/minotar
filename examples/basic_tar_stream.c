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
#include <unistd.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
    minotar_t* minotar;
    FILE* some_archive = NULL;
    char* file_name = NULL;
    char file_buf[256] = {0};
    size_t read_size = 0;
    char target_dir[] = "./";
    minotar_error_t err;
    
    if(argc < 2) {
        printf("Usage: minotar <filename>.tar\n");
        goto exit;
    }
    file_name = argv[1];
    
    if(access(file_name, F_OK) < 0) {
        printf("file <%s> does not exist. Cannot parse.\n", file_name);
        goto exit;
    }
    
    // initialize the library
    err = minotar_init(&minotar);
    if(err != MINOTAR_noerror) {
        printf("Minotar failed to initialize. (%d)\n", err);
        goto exit;
    }
    
    // tell minotar to decode to the local directory
    err = minotar_set_extract_directory(minotar, target_dir);
    if(err != MINOTAR_noerror) {
        printf("Minotar failed to set set directory. (%d)\n", err);
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
        err = minotar_decode(minotar, file_buf, read_size);
        if(err != MINOTAR_noerror) {
            printf("decode failed (%d).  exiting.\n", err);
            goto exit;
        }
    }
    
    printf("successfully decoded %s.  exiting.\n", file_name);
    
exit:
    // tear down the library
    minotar_deinit(&minotar);
    return 0;
}
