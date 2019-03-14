/*
 * UNG's Not GNU
 * 
 * Copyright (c) 2011, Jakob Kaivo <jakob@kaivo.net>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *chmod_desc = "change the file modes";
const char *chmod_inv  = "chmod [-R] mode file...";

#define USER	1 << 0
#define GROUP	1 << 1
#define OTHER	1 << 2
#define ALL	(USER | GROUP | OTHER)

static int recursive = 0;
static mode_t modeadd = 0;
static mode_t modesub = 0;
static mode_t modeabs = 0;
static mode_t absmask = 0;

static int _chmod (const char*);

int nftw_chmod (const char *p, const struct stat *sb, int typeflag, struct FTW *f)
{
  _chmod (p);
}

static int _chmod (const char *p)
{
  struct stat st;
  if (lstat (p, &st) != 0) {
    perror (p);
    return 1;
  }

printf ("Current: %04o\n", st.st_mode & 07777);
printf ("Add:     %04o\n", modeadd);
printf ("Sub:     %04o\n", modesub);
printf ("Abs:     %04o (%04o)\n", modeabs, absmask);

  // FIXME :(
  if (chmod (p, ((((st.st_mode | modeadd) ^ modesub) ^ absmask) | modeabs)) != 0)
    perror (p);
  return 0;
}

void chmod_set (int who, char rel, char mode)
{
  int bits = 0;
  int special = 0;
  int nmode = 0;
  if (mode == 'r') {
    bits = 4;
  } else if (mode == 'w') {
    bits = 2;
  } else if (mode == 'x') {
    bits = 1;
  } else if (mode == 's') {
    if (who & USER)
      special += 4;
    if (who & GROUP)
      special += 2;
    if (who & (USER | GROUP) == 0) {
      printf ("Must specify u or g for SUID or SGID\n");
      exit (1);
    }
  } else if (mode == 't') {
    if (who & OTHER) {
      special += 1;
    } else {
      printf ("Sticky only works for other\n");
      exit (1);
    }
  } else {
    printf ("Unknown mode: %c\n", mode);
    exit (1);
  }
  nmode = (special << 9) + (who & USER ? bits << 6 : 0) + (who & GROUP ? bits << 3 : 0) + (who & OTHER ? bits : 0);

  if (rel == '-')
    modesub |= nmode;
  else if (rel == '+')
    modeadd |= nmode;
  else if (rel == '=') {
    modeabs |= nmode;
    absmask |= (who & USER ? 0700 : 0) + (who & GROUP ? 070 : 0) + (who & OTHER ? 07 : 0);
  }
}

void chmod_parse (char *s)
{
  char *end;
  modeabs = strtol (s, &end, 8);
  if (end != NULL && strlen (end) > 0) {
    int i;
    int who = 0;
    char rel = '/';
    for (i = 0; i < strlen (end); i++) {
      if (who == 0 || rel == '/') {
        switch (end[i]) {
	  case 'u': who |= USER; break;
          case 'g': who |= GROUP; break;
          case 'o': who |= OTHER; break;
	  case 'a': who |= ALL; break;
	  case '=': if (who == 0) who |= ALL; rel = '='; break;
	  case '+': if (who == 0) who |= ALL; rel = '+'; break;
	  case '-': if (who == 0) who |= ALL; rel = '-'; break;
	  default: printf ("Unknown mode: %c\n", end[i]); exit(1);
        }
      } else if (end[i] == ',') {
        who = 0; rel = '/';
      } else {
        chmod_set (who, rel, end[i]);
      }
    }
  } else {
    absmask = 07777;
  }
}

int
main (int argc, char **argv)
{
  int c;

  while ((c = getopt (argc, argv, ":R")) != -1) {
    switch (c) {
      case 'R':
        recursive = 1;
        break;
      default:
        return 1;
    }
  }
  
  if (optind >= argc - 1)
    return 1;

  chmod_parse (argv[optind++]);

  while (optind < argc) {
    if (recursive)
      nftw (argv[optind], nftw_chmod, 1024, 0);
    else 
      _chmod (argv[optind]);
    optind++;
  }

  return 0;
}
