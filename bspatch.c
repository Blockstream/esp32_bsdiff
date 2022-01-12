/*-
 * Copyright 2003-2005 Colin Percival
 * Copyright 2012 Matthew Endsley
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <limits.h>

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#ifndef CONFIG_BSDIFF_BSPATCH_BUF_SIZE
#define CONFIG_BSDIFF_BSPATCH_BUF_SIZE (8 * 1024)
#endif


#include "bspatch.h"

/* CONFIG_BSDIFF_BSPATCH_BUF_SIZE can't be smaller than 8 */
#if 7 >= CONFIG_BSDIFF_BSPATCH_BUF_SIZE
#error "Error, CONFIG_BSDIFF_BSPATCH_BUF_SIZE can't be smaller than 8"
#endif

#define BUF_SIZE CONFIG_BSDIFF_BSPATCH_BUF_SIZE


#define min(A, B) ((A) < (B) ? (A) : (B))

static int64_t offtin(uint8_t *buf)
{
	int64_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

int bspatch(struct bspatch_stream_i *old, int64_t oldsize, struct bspatch_stream_n *new, int64_t newsize, struct bspatch_stream* stream)
{
	uint8_t buf[BUF_SIZE];
	int64_t oldpos,newpos;
	int64_t ctrl[3];
	int64_t i,k;
	int64_t towrite;
	const int64_t half_len = BUF_SIZE / 2;

	oldpos=0;newpos=0;
	while(newpos<newsize) {
		/* Read control data */
		for(i=0;i<=2;i++) {
			if (stream->read(stream, buf, 8))
				return -1;
			ctrl[i]=offtin(buf);
		}

		/* Sanity-check */
		if (ctrl[0]<0 || ctrl[0]>INT_MAX ||
			ctrl[1]<0 || ctrl[1]>INT_MAX ||
			newpos+ctrl[0]>newsize)
			return -1;

		/* Read diff string and add old data on the fly */
		i = ctrl[0];
		while (i) {
			towrite = min(i, half_len);
			if (stream->read(stream, &buf[half_len], towrite))
				return -1;
			if (old->read(old, buf, oldpos + (ctrl[0] - i), towrite))
				return -1;
			for(k=0;k<towrite;k++)
				buf[k + half_len] += buf[k];
			if (new->write(new, &buf[half_len], towrite))
				return -1;
			i -= towrite;
		}

		/* Adjust pointers */
		newpos+=ctrl[0];
		oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>newsize)
			return -1;

		/* Read extra string and copy over to new on the fly*/
		i = ctrl[1];
		while (i) {
			towrite = min(i, BUF_SIZE);
			if (stream->read(stream, buf, towrite))
				return -1;
			if (new->write(new, buf, towrite))
				return -1;
			i -= towrite;
		}

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

	return 0;
}

#if defined(BSPATCH_EXECUTABLE)

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static int __read(const struct bspatch_stream* stream, void* buffer, int length)
{
	if (fread(buffer, 1, length, (FILE*)stream->opaque) != length) {
		return -1;
	}
	return 0;
}

static int old_read(const struct bspatch_stream_i* stream, void* buffer, int pos, int length) {
	uint8_t* old;
	old = (uint8_t*)stream->opaque;
	memcpy(buffer, old + pos, length);
	return 0;
}

struct NewCtx {
	uint8_t* new;
	int pos_write;
};

static int new_write(const struct bspatch_stream_n* stream, const void *buffer, int length) {
	struct NewCtx* new;
	new = (struct NewCtx*)stream->opaque;
	memcpy(new->new + new->pos_write, buffer, length);
	new->pos_write += length;
	return 0;
}

int main(int argc,char * argv[])
{
	FILE * f;
	int fd;
	uint8_t header[24];
	uint8_t *old, *new;
	int64_t oldsize, newsize;
	struct bspatch_stream stream;
	struct bspatch_stream_i oldstream;
	struct bspatch_stream_n newstream;
	struct stat sb;

	if(argc!=5) errx(1,"usage: %s oldfile newfile newsize patchfile\n",argv[0]);

	newsize = atoi(argv[3]);

	/* Open patch file */
	if ((f = fopen(argv[4], "r")) == NULL)
		err(1, "fopen(%s)", argv[4]);

	/* Close patch file and re-open it at the right places */
	if(((fd=open(argv[1],O_RDONLY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(fstat(fd, &sb)) ||
		(close(fd)==-1)) err(1,"%s",argv[1]);
	if((new=malloc(newsize+1))==NULL) err(1,NULL);

	stream.read = __read;
	oldstream.read = old_read;
	newstream.write = new_write;
	stream.opaque = f;
	oldstream.opaque = old;
	struct NewCtx ctx = { .pos_write = 0, .new = new };
	newstream.opaque = &ctx;
	if (bspatch(&oldstream, oldsize, &newstream, newsize, &stream))
		errx(1, "bspatch");

	/* Clean up */
	fclose(f);

	/* Write the new file */
	if(((fd=open(argv[2],O_CREAT|O_TRUNC|O_WRONLY,sb.st_mode))<0) ||
		(write(fd,new,newsize)!=newsize) || (close(fd)==-1))
		err(1,"%s",argv[2]);

	free(new);
	free(old);

	return 0;
}

#endif
