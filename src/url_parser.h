//
// Copyright (c) 2023 Brian Sullender
// All rights reserved.
//
// This source code is licensed under the following terms:
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// (1) If any part of the source code or algorithm is used, it is requested that
//     appropriate attribution is given to the authors by providing a link to
//     the original repository or a reference to the authors name.
// (2) Permission for use of this software is granted only if the user accepts
//     full responsibility for any undesirable consequences; the authors accept
//     NO LIABILITY for damages of any kind.
// 
// By using this software, you acknowledge that you have read and understood
// // these terms and agree to abide by them.
//
// https://github.com/b-sullender/url-parser
//

#ifndef __URL_PARSER_H__
#define __URL_PARSER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// These are the spec maximums, but they're silly here
// #define MAX_URL_SCHEME_LEN 32
// #define MAX_URL_AUTHORITY_LEN 512
// #define MAX_URL_PORT_LEN 64
// #define MAX_URL_PATH_LEN 1024
// #define MAX_URL_QUERY_LEN 1024
// #define MAX_URL_FRAGMENT_LEN 256

#define MAX_URL_SCHEME_LEN 10
#define MAX_URL_AUTHORITY_LEN 100
#define MAX_URL_PORT_LEN 10
#define MAX_URL_PATH_LEN 256
#define MAX_URL_QUERY_LEN 256
#define MAX_URL_FRAGMENT_LEN 100



typedef struct url_parts {
    char scheme[MAX_URL_SCHEME_LEN];
    char authority[MAX_URL_AUTHORITY_LEN];
    char port[MAX_URL_PORT_LEN];
    int port_number;
    char path[MAX_URL_PATH_LEN];
    char query[MAX_URL_QUERY_LEN];
    char fragment[MAX_URL_FRAGMENT_LEN];
} URL_PARTS;

// ParseURL
// This function parses a URL string
// @URL: The URL string the parse
// @pURL_Parts: A pointer to a URL_PARTS struct to fill
// @return: true on success, false otherwise
bool parse_url(char *url, URL_PARTS *url_parts);

// PrintURL
// This function prints each part of a URL_PARTS struct individually,
// and the entire URL on a single line. For testing purposes only
// @pURL_Parts: A pointer to a URL_PARTS struct to print
void print_url(URL_PARTS *url_parts);

#endif