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


#ifndef MINOTAR_TAR_DEFINITIONS_H
#define MINOTAR_TAR_DEFINITIONS_H

typedef enum tar_file_type_ {
    FILE_TYPE_normal_file_alt='\0',
    FILE_TYPE_normal_file='0',
    FILE_TYPE_hard_link='1',
    FILE_TYPE_symlink='2',
    FILE_TYPE_char_special='3',
    FILE_TYPE_block_special='4',
    FILE_TYPE_directory='5',
    FILE_TYPE_fifo='6',
    FILE_TYPE_continuous_file='7'
} tar_file_type_t;

struct header_posix_ustar {
    char	       name[100];
    char	       mode[8];
    char	       uid[8];
    char	       gid[8];
    char	       size[12];
    char	       mtime[12];
    char	       checksum[8];
    tar_file_type_t	typeflag;
    char	       linkname[100];
    char	       magic[6];
    char	       version[2];
    char	       uname[32];
    char	       gname[32];
    char	       devmajor[8];
    char	       devminor[8];
    char	       prefix[155];
    char	       pad[12];
};

#endif // MINOTAR_TAR_DEFINITIONS_H
