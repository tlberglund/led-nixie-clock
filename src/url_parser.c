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

#include "url_parser.h"

bool parse_url(char *url, URL_PARTS *url_parts)
{
   int i;
   unsigned char c;
   bool b = false;

   // Clear the data
   memset(url_parts, 0, sizeof(URL_PARTS));

   // Setup our parser pointer
   char *string = url;

   // Check for the authority preceding double slash
   char *authority = strstr(string, "//");
   if(authority == NULL)
   {
      authority = string;
   }
   else
   {
      // Parse the sheme
      i = 0;
      while(string < authority)
      {
         unsigned char c = *string;

         if(c == ':')
         {
            // End of scheme
            break;
         }
         if(c == '/')
         {
            // End of scheme
            break;
         }
         // Check for overflow
         if(i == (MAX_URL_SCHEME_LEN - 1))
         {
            return false;
         }
         url_parts->scheme[i] = c;
         string++;
         i++;
      }

      // Skip the slashes
      authority += 2;
   }

   c = *authority;
   if(c == '[')
   {
      b = true;
      authority++;
   }

   // Parse the authority
   i = 0;
   while((c = *authority) != 0)
   {
      // Skip spaces
      if(c == ' ')
      {
         authority++;
         continue;
      }

      // Check for path
      if(c == '/')
      {
         break;
      }

      // Check for port number
      if(c == ':')
      {
         // Check if bracket used (IPv6)
         if(!b)
         {
            break;
         }
      }

      // Check for query
      if(c == '?')
      {
         break;
      }

      // Check for fragment
      if(c == '#')
      {
         break;
      }

      // Should we check for closing bracket?
      if(b)
      {
         if(c == ']')
         {
            authority++;
            break;
         }
      }

      // Check for overflow
      if(i == (MAX_URL_AUTHORITY_LEN - 1))
      {
         return false;
      }

      url_parts->authority[i] = c;
      authority++;
      i++;
   }

   // Switch back to string variable
   string = authority;

   // Check for a port number
   if(*string == ':')
   {
      string++;
     
      // Parse the port number
      i = 0;
      while((c = *string) != 0)
      {
         // Check for path
         if(c == '/')
         {
            break;
         }

         // Check for query
         if(c == '?')
         {
            break;
         }

         // Check for fragment
         if(c == '#')
         {
            break;
         }

         // Check for overflow
         if(i == (MAX_URL_PORT_LEN - 1))
         {
            return false;
         }

         url_parts->port[i] = c;
         string++;
         i++;
      }
   }

   // Convert port number if it exists
   if(strlen(url_parts->port)) {
      url_parts->port_number = atoi(url_parts->port);
   }
   else {
      url_parts->port_number = -1;
   }

   // Check for a path
   if(*string == '/')
   {
      // Parse the path
      i = 0;
      while((c = *string) != 0)
      {
         // Check for query
         if(c == '?')
         {
            break;
         }

         // Check for fragment
         if(c == '#')
         {
            break;
         }

         // Check for overflow
         if(i == (MAX_URL_PATH_LEN - 1))
         {
            return false;
         }

         url_parts->path[i] = c;
         string++;
         i++;
      }
   }

   // Check for a query
   if(*string == '?')
   {
      // Parse the query
      i = 0;
      while((c = *string) != 0)
      {
         // Check for fragment
         if(c == '#')
         {
            break;
         }

         // Check for overflow
         if(i == (MAX_URL_QUERY_LEN - 1))
         {
            return false;
         }

         url_parts->query[i] = c;
         string++;
         i++;
      }
   }

   // Check for a fragment
   if(*string == '#')
   {
      // Parse the fragment
      i = 0;
      while((c = *string) != 0)
      {
         // Check for overflow
         if(i == (MAX_URL_FRAGMENT_LEN - 1))
         {
            return false;
         }

         url_parts->fragment[i] = c;
         string++;
         i++;
      }
   }

   return true;
}


void print_url(URL_PARTS *url_parts)
{
   // Safety check
   if(url_parts == NULL)
   {
      fprintf(stderr, "%s\n", "PrintURL Error: Parameter 'pURL_Parts' is NULL");
      return;
   }

   // Print each individually
   printf("Scheme: %s\nAuthority: %s\nPort: %s\nPath: %s\nQuery: %s\nFragment: "
          "%s\n",
          url_parts->scheme, 
          url_parts->authority, 
          url_parts->port,
          url_parts->path, 
          url_parts->query, 
          url_parts->fragment);

   // Print as a single URL
   if(strlen(url_parts->scheme))
   {
      printf("%s://%s", url_parts->scheme, url_parts->authority);
   }
   else if(strlen(url_parts->authority))
   {
      printf("%s", url_parts->authority);
   }
   if(strlen(url_parts->port))
   {
      printf(":%s", url_parts->port);
   }
   if(strlen(url_parts->path))
   {
      printf("%s", url_parts->path);
   }
   if(strlen(url_parts->query))
   {
      printf("%s", url_parts->query);
   }
   if(strlen(url_parts->fragment))
   {
      printf("%s", url_parts->fragment);
   }

   printf("%s", "\n");
}
