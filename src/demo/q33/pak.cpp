/* Aftershock 3D rendering engine
 * Copyright (C) 1999 Stephen C. Taylor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "util.h"
#include "pak.h"
#include "unzip.h"
//#include <string.h>
#include <stdio.h>

#include "arDataUtilities.h"
extern string pakFilePath;
#include <iostream>

/*
 * These are facilities to manipulate pk3 files.  It uses Gilles Vollant's
 * unzip extensions to zlib (source included with distribution).
 */

static unzFile pakfile[10];
static int numPaks = 0;
static int currPak = -1; // -1 means "no current pak file"
static FILE *currFile = NULL; // NULL means "no current file"

void
pak_openpak(const char *path)
{
  char bName[1024];
  // Try to find the file in $PWD.
  pakfile[numPaks] = unzOpen(path);
  // Wasn't in $PWD, so step through the data path looking for pak files.
  if (!pakfile[numPaks]){
    // PLEASE NOTE: If this code is compiled/linked debug on Win32, then
    // on certain Windows versions, there may be a segfault on return from
    // this function. Why? This is the first return from an szg dll into
    // the local code w/ an STL object.
    string filePath = ar_fileFind(path,"quake",pakFilePath);
    if (filePath != "NULL") {
      cerr << "q33 remark: found " << filePath << endl;
      // file was found
      //ar_stringToBuffer(filePath, bName, 1024);
      pakfile[numPaks] = unzOpen(filePath.c_str());
    } else {
      cerr << "q33 remark: Didn't find " 
           << path << " in subdirectory 'quake'.\n";
      filePath = ar_fileFind(path,"q33",pakFilePath);
      if (filePath != "NULL") {
        cerr << "q33 remark: found " << filePath << endl;
        // file was found
        char bName2[1024];
        ar_stringToBuffer(filePath, bName2, 1024);
        pakfile[numPaks] = unzOpen(bName2);
      } else {
        cerr << "q33 remark: Didn't find " 
             << path << " in subdirectory 'q33'.\n";
      }
    }
  }

  if (!pakfile[numPaks])
    Error("Failed to open pk3 file %s\n", path);
  numPaks++;
}

void
pak_closepak(void)
{
    for (int i=0; i < numPaks; i++) {
      if (pakfile[i])
	unzClose(pakfile[i]);
      pakfile[i] = NULL;
    }
}

int
pak_open(const char *path)
{
   int i;
   if((currFile = fopen(path, "r"))) { // Double parentheses to shut up GCC
       currPak = -1;
       return 1; // Found matching unpak'ed file - look no further
   }
   for (i=0; i < numPaks; i++) {
     if (unzLocateFile(pakfile[i], path, 2) == UNZ_OK &&
         unzOpenCurrentFile(pakfile[i]) == UNZ_OK) {
         currPak = i;
         return 1; // Found match in pak file
     }
   }
   return 0; // File not found
}

void
pak_close(void)
{
    if(currPak>=0) // Do not try to close pak if none was opened
	unzCloseCurrentFile(pakfile[currPak]);
    else
	fclose(currFile);
}

int
pak_read(void *buf, uint_t size, uint_t nmemb)
{
    int len;

    if(currPak >=0) {
	len = unzReadCurrentFile(pakfile[currPak], buf, size * nmemb);
	return len / size;
    }
    else {
	len = fread(buf, size, nmemb, currFile);
	return len / size;
    }
}

uint_t
pak_getlen(void)
{
    unz_file_info info;

    if(currPak >=0) {
	if (unzGetCurrentFileInfo(pakfile[currPak], &info, NULL, 0, NULL, 0, NULL, 0)
	    != UNZ_OK)
	    return 0;
	return info.uncompressed_size;
    }
    else {
	uint_t len;
	// It seems awkward to ask for two positionings (seek+rewind)
	// when a simple file size info is all we need. Better way?
	(void)fseek(currFile, 0, SEEK_END);
	len = (uint_t)ftell(currFile);
	rewind(currFile);
	return len;
    }
}

uint_t
pak_readfile(const char *path, uint_t bufsize, byte_t *buf)
{
    uint_t len;
    
    if (!pak_open(path))
	return 0;

    if ((len = pak_getlen()) != 0)
    {
	if (pak_read((void*)buf, 1, len) != len)
	    len = 0;
    }
    pak_close();    
    return len;
}

uint_t
pak_listshaders(uint_t bufsize, char *buf)
{
    // FIXME: Include shaders from outside of pak files! (scripts/*.shader)
    uint_t num = 0, len;
    int status, i;
    char *end = buf + bufsize;

    for (i=0; i < numPaks; i++) {
      status = unzGoToFirstFile(pakfile[i]);
      while (status == UNZ_OK)
      {
  	unzGetCurrentFileInfo(pakfile[i], NULL, buf, 64, NULL, 0, NULL, 0);
	len = strlen(buf);
	if (len >= 7 && strcmp(&buf[len-7], ".shader") == 0)
	{
	    num++;
	    buf += len + 1;
	    if (buf > end)
		Error("pak_listshaders(): buffer overrun");
	}
	status = unzGoToNextFile(pakfile[i]);
      }
    }

    return num;
}

/* not used */
#if 0
int
pak_checkfile(const char *path)
{
    int status;

    status = unzLocateFile(pakfile, path, 2);
    return (status == UNZ_OK);
}
#endif
