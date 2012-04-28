
/*
 * aar.c a program to read and write alto file systems L. Stewart 12-3-92
 */

/*
 * Modifications L. Stewart 1/18/93 add z switch to read compressed files
 */

/*
 * endian issues: for big endian machines, reverse all 16-bit shorts in the
 * file read in, then do not reverse 16-bit shorts in the strings
 */

#include <fcntl.h>
/*#include <sys/stat.h>*/
/*#include <sys/types.h>*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <time.h>


#define NPAGES 4872

typedef unsigned short word;
typedef unsigned char byte;

struct PAGE
{
  word pagenum;
  word header[2];
  word label[8];
  word data[256];
};

struct LABEL
{
  word nextRDA;
  word prevRDA;
  word unused1;			/* always 0 ? */
  word nbytes;			/* up to 512 */
  word filepage;		/* from 0 */
  word fid[3];			/* see below */
};

struct TIME
{
  word altotime[2];
};

struct SN
{
  word sn[2];
};

struct FP
{
  struct SN serialNumber;
  word version;
  word blank;
  word leaderVDA;
};

struct FA
{
  word vda;
  word pageNumber;
  word charPos;
};

struct LEADER
{
  struct TIME created;
  struct TIME written;
  struct TIME read;
  char filename[40];
  word leaderProps[210];
  word spare[10];		/* for 256 word pages */
  byte proplength;		/* 1 word */
  byte propbegin;
  byte changeSN;		/* 1 word */
  byte consecutive;
  struct FP dirFpHint;		/* 5 words */
  struct FA lastPageHint;	/* 3 words */
};

/* fid[0] is 1 for all used files, ffff for free pages */
/* fid[1] is 8000 for a directory, 0 for regular, ffff for free */
/* fid[2] is the fileid */

struct DV
{
  word typelength;
  struct FP fileptr;
  char filename[40];		/* not all used */
};

/* header for the DiskDescriptor file */
struct KDH
{
  word nDisks;			/* how many disks in the fs */
  word nTracks;			/* how big is each disk */
  word nHeads;			/* how many heads */
  word nSectors;		/* how many sectors */
  struct SN lastSN;		/* last SN used on disk */
  word blank;			/* formerly bitTableChanged */
  word diskBTsize;		/* number valid words in the bit table */
  word defVersionsKept;		/* 0 implies no multiple versions */
  word freePages;		/* pages left */
  word blank1[6];
};

/* storage for disk allocation datastructures */
struct KDH kdh;			/* disk descriptor */
word *bitTable;			/* pages allocated */

/* storage for the disk image for dp0 and dp1 */

struct PAGE disk[NPAGES * 2];

extern void dump_headers ();
extern void dump_directory ();
extern void
  extract_file (int leader_page_number);

extern void
  extract_files (int argc, char *argv[]);

extern void
  table_files (int argc, char *argv[]);

extern int
  find_file (char *name);

extern void dump_leader_pages ();
extern int Verify_Headers ();

extern word
  RDAtoVDA (word);

extern word
  VDAtoRDA (word);

extern void
  copystring (char *to, char *from, int count, int lower);

extern void
  swabit (char *data, int count);

extern int
  getword (struct FA *fa);

extern int ValidateDiskDescriptor ();

extern void FixDiskDescriptor ();

extern int
  getBT (int page);		/* get bit from free page bit table */

extern void
  setBT (int page, int new);	/* set bit in bit table */

extern void
  delete_file (int leader_page_VDA);

extern void extract_all_files ();

extern void
  print_file_times (int leader_page_VDA);

extern void
  table_file (int leader_page_VDA, struct DV *dv);

extern void table_all_files ();
extern void print_alto_time ();

extern void
  ReadDiskFile (char *name);

extern void
  ReadSingleDisk (char *name, struct PAGE *diskp);

/* general utilities */
int Assert (int book, char *errmsg,...);	/* printf style */

void
  AssertOrDie (int book, char *errmsg,...);	/* printf style */

/* actual procedures */
struct LEADER *
pageLeader (int vda)
{
  return ((struct LEADER *) &disk[vda].data[0]);
}

struct LABEL *
pageLabel (int vda)
{
  return ((struct LABEL *) &disk[vda].label[0]);
}


union endian {
  short int e;
  unsigned char l, h;
};

union endian little;		/* endian test */

int lflag = 0;			/* dump leader pages */
int xflag = 0;			/* extract files */
int tflag = 0;			/* list files */
int vflag = 0;			/* verbose mode */
int bflag = 0;			/* extract binary file */
int zflag = 0;			/* work from compressed disk image */
int sflag = 0;			/* swab words on extract */
int doubledisk = 0;		/* double disk system */

int
main (int argc, char *argv[])
{
  /* process arguments */
  /* aar [xt][v] diskimage [file...] */
  /* new flag 'b' for binary, which applies to extract */
  /* new flag 's' for swab on extract */
  char *flags;

  /* initialize for endian test */
  little.e = 1;
  if (argc < 3)
    {
      printf ("Usage: aar  [xt][v] diskimage [file...] \n");
      exit (1);
    }
  flags = argv[1];
  while (*flags)
    {
      switch (*flags)
	{
	case 'l':
	  lflag++;
	  break;
	case 't':
	  tflag++;
	  break;
	case 'x':
	  xflag++;
	  break;
	case 'v':
	  vflag++;
	  break;
	case 'b':
	  bflag++;
	  break;
	case 'z':
	  zflag++;
	  break;
	case 's':
	  sflag++;
	  break;
	default:
	  AssertOrDie (0, "Unknown flag %c\n", *flags);
	  break;
	}
      flags++;
    }
  AssertOrDie (!(tflag && xflag), "Illegal flag combination\n");
  ReadDiskFile (argv[2]);
  if (little.l == 0)
    swabit ((char *) disk, sizeof (disk));

/*  AssertOrDie (Verify_Headers (), "Disk Scrambled, header verify failed\n"); */
  Assert(Verify_Headers (), "Disk Scrambled, header verify failed\n");

/*  ValidateDiskDescriptor (); */
  if (lflag)
    dump_leader_pages ();
  if (tflag)
    table_files (argc, argv);
  if (xflag)
    extract_files (argc, argv);
  return 0;
}



/******************************/
/* Main work-doing procedures */
/******************************/


void
ReadDiskFile (char *name)
{
  char *dp0name = NULL;
  char *dp1name = NULL;
  dp0name = name;
  dp1name = strchr (name, ',');
  if (dp1name != NULL)
    {
      *dp1name = 0;
      dp1name += 1;
    }
  ReadSingleDisk (dp0name, &disk[0]);
  doubledisk = dp1name != NULL;
  if (doubledisk)
    ReadSingleDisk (dp1name, &disk[NPAGES]);
}

void
ReadSingleDisk (char *name, struct PAGE *diskp)
{
  FILE *infile;
  int bytes;
  int totalbytes = 0;
  int total = NPAGES * sizeof (struct PAGE);
  char *dp = (char *) diskp;
  /*
   * We conclude the disk image is compressed if either the zflag is set or
   * if the name ends with .Z
   */
  if (zflag || (strstr (name, ".Z") == (name + strlen (name) - 2)))
    {
      char *cmd;
      cmd = malloc (strlen (name) + 10);
      sprintf (cmd, "zcat %s", name);
      infile = popen (cmd, "r");
      AssertOrDie (infile != NULL,
		   "popen failed on zcat %s\n", name);
      free (cmd);
    }
  else
    {
      infile = fopen (name, "rb");
      AssertOrDie (infile != NULL,
		   "open failed on Alto disk image file %s\n", name);
    }
  while (totalbytes < total)
    {
      bytes = fread (dp, sizeof (char), total - totalbytes, infile);
      dp += bytes;
      totalbytes += bytes;
      Assert (!(ferror (infile) || feof (infile)),
	      "disk read failed: %d bytes read instead of %d\n",
	      totalbytes, total);
    }
}

void
dump_disk_block (int page)
{
  int row, col;
  word d;
  char str[17], c;
  for (row = 0; row < 16; row += 1)
    {
      printf ("%04x:", row * 8);
      for (col = 0; col < 8; col += 1)
	{
	  printf (" %04x", disk[page].data[(row * 8) + col]);
	}
      for (col = 0; col < 8; col += 1)
	{
	  d = disk[page].data[(row * 8) + col];
	  c = (d >> 8) & 0x7f;
	  str[(col * 2)] = (isprint ((unsigned char)c)) ? c : ' ';
	  c = (d) & 0x7f;
	  str[(col * 2) + 1] = (isprint ((unsigned char)c)) ? c : ' ';
	}
      str[16] = 0;
      printf ("  %16s\n", str);
    }

}

void
dump_leader_pages ()
{
  int i, bad, length, last;
  char fn[42];
  struct LABEL *l;
  struct LEADER *lp;
  bad = 0;
  last = (doubledisk) ? NPAGES * 2 : NPAGES;
  for (i = 0; i < last; i += 1)
    {
      l = pageLabel (i);
      lp = pageLeader (i);
      if ((l->filepage == 0) && (l->fid[0] == 1))
	{
	  copystring (fn, lp->filename, 40, 0);
	  length = fn[0];
	  if (length > 39)
	    length = 39;
	  length -= 1;		/* erase closing '.' */
	  fn[length + 1] = 0;
	  if (!vflag)
	    printf ("%s\n", &fn[1]);
	  else
	    {
	      /* time conversion here */
	      printf ("%s ", &fn[1]);
	      print_file_times (i);
	      printf ("\n");
/*	      dump_disk_block (i); */
	    }
	}
    }
}


void
dump_headers ()
{
  int i, j, last;
  last = (doubledisk) ? NPAGES * 2 : NPAGES;
  for (i = 0; i < last; i += 1)
    {
      printf ("%04x-%04x %04x", disk[i].pagenum,
	      disk[i].header[0], disk[i].header[1]);
      for (j = 0; j < 8; j += 1)
	printf ("-%04x", disk[i].label[j]);
      printf ("\n");
    }
}

char *monthnames[12] =
{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void
print_alto_time (struct TIME t)
{
  time_t time;
  struct tm *ltm;
  time = t.altotime[1] + (t.altotime[0] << 16);
  time += 2117503696;		/* magic value to convert to Unix epoch */
  ltm = localtime (&time);
  /* like  4-Jun-80  17:14:36  */
  printf ("%02d-%s-%04d  %2d:%02d:%02d", ltm->tm_mday, monthnames[ltm->tm_mon],
	  1900 + ltm->tm_year, ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
}

void
print_file_times (int leader_page_VDA)
{
  struct LABEL *l;
  struct LEADER *lp;
  l = pageLabel (leader_page_VDA);
  lp = pageLeader (leader_page_VDA);
  printf ("cr: ");
  print_alto_time (lp->created);
  printf (" wr: ");
  print_alto_time (lp->written);
  printf (" rd: ");
  print_alto_time (lp->read);
}

void
dump_directory ()
{
  int i, w, length, valid;
  struct LABEL *l;
  struct FA fa;
  struct DV *dv;
  char fn[42];
  word dvspace[sizeof (struct DV) / 2];
  l = pageLabel (1);
  fa.vda = RDAtoVDA (l->nextRDA);
  fa.pageNumber = 1;
  fa.charPos = 0;
  dv = (struct DV *) &dvspace[0];
  for (;;)
    {
      w = getword (&fa);
      if (w < 0)
	return;			/* EOF on directory */
      dvspace[0] = w;
      valid = (((dv->typelength >> 10) & 0x3f) == 1);
      for (i = 1; i < (dvspace[0] & 0x3ff); i += 1)
	{
	  w = getword (&fa);
	  AssertOrDie (w >= 0,
		       "unexpected EOF on directory!\n");
	  if (valid && (i < (sizeof (struct DV) / 2)))
	      dvspace[i] = w;
	}
      if (valid)
	{
	  copystring (fn, dv->filename, 40, 0);
	  length = fn[0];
	  if (length > 39)
	    length = 39;
	  length -= 1;		/* erase final . */
	  fn[length + 1] = 0;
	  if (!vflag)
	    printf ("%s\n", &fn[1]);
	  else
	    {
	      printf ("%20s ", &fn[1]);
	      print_file_times (dv->fileptr.leaderVDA);
	      printf ("\n");
	    }
	}
    }
}

int
file_length (int leader_page_VDA)
{
  int length = 0;
  int filepage;
  struct LABEL *label;
  filepage = leader_page_VDA;
  while (filepage != 0)
    {
      label = pageLabel (filepage);
      length = length + label->nbytes;
      if (label->nbytes < 512)
	break;
      filepage = RDAtoVDA (label->nextRDA);
    }
  return (length);
}

void
name_from_dv (struct DV *dv, char *fn)
{
  char myfn[42];
  int length;
  copystring (myfn, dv->filename, 40, 0);
  length = myfn[0];
  if (length > 39)
    length = 39;
  length -= 1;			/* erase final . */
  myfn[length + 1] = 0;
  strcpy (fn, &myfn[1]);
}

void
name_from_leader (struct LEADER *lp, char *fn)
{
  char myfn[42];
  int length;
  copystring (myfn, lp->filename, 40, 0);
  length = myfn[0];
  if (length > 39)
    length = 39;
  length -= 1;			/* erase final . */
  myfn[length + 1] = 0;
  strcpy (fn, &myfn[1]);
}

void
table_file (int leader_page_VDA, struct DV *dv)
{
  struct LEADER *lp;
  int length;
  char fn[42];
  if (dv)
    leader_page_VDA = dv->fileptr.leaderVDA;
  lp = pageLeader (leader_page_VDA);
  if (vflag)
    {
      length = file_length (leader_page_VDA);
      printf ("%8d ", length);
      /* printf("create: "); */
      print_alto_time (lp->created);
      /*
     * printf(" written: "); print_alto_time(lp->written); printf(" read: ");
     * print_alto_time(lp->read);
     */
    }
  if (dv)
    name_from_dv (dv, fn);	/* print file name from dv */
  else
    name_from_leader (lp, fn);	/* print file name from leader page */
  printf (" %s\n", fn);
}

void
extract_files (int argc, char *argv[])
{
  int i, lp;
  if (argc == 3)
    extract_all_files ();
  else
    {
      for (i = 3; i < argc; i += 1)
	{
	  lp = find_file (argv[i]);
	  extract_file (lp);
	}
    }
}

void
table_files (int argc, char *argv[])
{
  int i, lp;
  if (argc == 3)
    table_all_files ();
  else
    {
      for (i = 3; i < argc; i += 1)
	{
	  lp = find_file (argv[i]);
	  /* table_file(lp); */
	}
    }
}

void
delete_files (int argc, char *argv[])
{
  int i, lp;
  AssertOrDie (argc > 3, "delete command must specify files\n");
  for (i = 3; i < argc; i += 1)
    {
      lp = find_file (argv[i]);
      delete_file (lp);
    }
}

/**********************************/
/* general disk untility routines */
/**********************************/

word
RDAtoVDA (word rda)
{
  word vda, head, sector, cylinder;
  sector = ((rda >> 12) & 0xf);
  head = ((rda >> 2) & 1);
  cylinder = ((rda >> 3) & 0x1ff);
  vda = (cylinder * 24) + (head * 12) + sector;
  if(rda & 2) vda += NPAGES;
  return (vda);
}

word
VDAtoRDA (word vda)
{
  int rda;
  int head, sector, cylinder;
  sector = vda % 12;
  head = (vda / 12) & 1;
  cylinder = vda / 24;
  rda = (cylinder << 3) + (head << 2) + (sector << 12);
  if(vda >= NPAGES) rda |= 2;
  return (rda);
}

int
find_file (char *name)
{
  /* search directory for file <name> and return leader page VDA */
  int i, length, last;
  char fn[42], *s;
  struct LABEL *l;
  struct LEADER *lp;
  /* use linear search ! */
  last = (doubledisk) ? NPAGES * 2 : NPAGES;
  for (i = 0; i < last; i += 1)
    {
      l = pageLabel (i);
      lp = pageLeader (i);
      if ((l->filepage == 0) && (l->fid[0] == 1))
	{
	  copystring (fn, lp->filename, 40, 1);
	  length = fn[0];
	  if (length > 39)
	    length = 39;
	  length -= 1;		/* erase closing '.' */
	  fn[length + 1] = 0;
	  s = name;
/*
	  while (*s)
	    {
	      *s = tolower (*s);
	      s++;
	    }
	  if (strcmp (name, &fn[1]) == 0)
	    return (i);
*/
	}
    }
  /* AssertOrDie(0, "file %s not found\n", name); */
  return (-1);
}


void
delete_file (int leader_page_VDA)
{
}


void
extract_all_files ()
{
  int i, last;
  struct LABEL *l;
  last = (doubledisk) ? NPAGES * 2 : NPAGES;
  for (i = 0; i < last; i += 1)
    {
      l = (struct LABEL *) &disk[i].label[0];
      if ((l->filepage == 0) && (l->fid[0] == 1))
	extract_file (i);
    }
}


void
  table_all_files ()
{	
	int i, last;
	struct LABEL *l;
	last = (doubledisk) ? NPAGES * 2 : NPAGES;
	for (i = 0; i < last; i += 1)
  {
	  l = (struct LABEL *) &disk[i].label[0];
	  if ((l->filepage == 0) && (l->fid[0] == 1))
    {	
	    printf("%8d ", i);
	    table_file(i, NULL);
    }
    }
}

void
extract_file (int leader_page_VDA)
{
  int length;
  int ofd;
  char fn[42];
  struct LABEL *l;
  struct LEADER *lp;
  int filepage, bytes;
  l = pageLabel (leader_page_VDA);
  lp = pageLeader (leader_page_VDA);
  AssertOrDie (l->filepage == 0,
	       "extract_file, page %d is not a leader page!\n",
	       leader_page_VDA);
  copystring (fn, lp->filename, 40, 0);
  length = fn[0];
  if (length > 39)
    length = 39;
  length -= 1;			/* erase final . */
  fn[length + 1] = 0;
  if (vflag)
    printf ("x %s\n", &fn[1]);
  ofd = open (&fn[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
  AssertOrDie (ofd >= 0, "open for write failed on %s\n", &fn[1]);
  while (l->nextRDA != 0)
    {
      filepage = RDAtoVDA (l->nextRDA);

      l = pageLabel (filepage);
      if (sflag) {
        char data[256 * 2];
        char *src = (char *)&disk[filepage].data;
        int i;
        for (i = 0; i < 256 * 2; i++)
           data[i] = src[i ^ 1];
        bytes = write (ofd, data, l->nbytes);
      } else {
        bytes = write (ofd, (char*)&disk[filepage].data[0], l->nbytes);
      }
      AssertOrDie (bytes == l->nbytes, "write to %s failed!\n", &fn[1]);
    }
  close (ofd);
}


int
altotometotime (struct TIME at)
{
  return 0;
}

int
getword (struct FA *fa)
{
  struct LABEL *l;
  int w;
  l = pageLabel (fa->vda);
  AssertOrDie ((fa->charPos & 1) == 0, "getword called on odd byte boundary\n");
  if (fa->charPos >= l->nbytes)
    {
      if ((l->nextRDA == 0) || (l->nbytes < 512))
	return (-1);
      fa->vda = RDAtoVDA (l->nextRDA);
      l = pageLabel (fa->vda);
      fa->pageNumber += 1;
      fa->charPos = 0;
    }
  AssertOrDie (fa->pageNumber == l->filepage,
	       "disk corruption - expected vda %d to be filepage %d\n",
	       fa->vda, l->filepage);
  w = disk[fa->vda].data[fa->charPos >> 1];
  fa->charPos += 2;
  return (w);
}

/* don't think we need this routine anyway */
void
putword (struct FA *fa, word w)
{
  struct LABEL *l;
  l = pageLabel (fa->vda);
  AssertOrDie ((fa->charPos & 1) == 0, "putword called on odd byte boundary\n");
  /*
   * case 1: writing in the middle of an existing file, on a page with more
   * bytes than the one we're at case 2: extending the last page of a file,
   * changing nbytes as we go case 3: extending past the last page, need to
   * allocate a new one
   */
  /* case 1, existing page, in the middle */
}


/**********************************************/
/* Disk page allocation, DiskDescriptor, etc. */
/**********************************************/

int
getBT (int page)
{
  int bit;
  /*
   * the bit table is big endian, so page 0 is in bit 15, page 1 is in bit
   * 15, and page 15 is in bit 0
   */
  bit = 15 - (page % 16);
  return ((bitTable[page / 16] >> bit) & 1);
}

void
setBT (int page, int new)
{
  int w, bit;
  w = page / 16;
  bit = 15 - (page % 16);
  bitTable[w] &= ~(1 << bit);
  bitTable[w] |= (new != 0) << bit;
}

int
pagefree (int page)
{
  struct LABEL *l;
  l = pageLabel (page);
  return ((l->fid[0] == 0xffff) && (l->fid[1] == 0xffff) &&
	  (l->fid[2] == 0xffff));
}

/* Sanity Checking */

/* make sure that each page header refers to itself */
int
Verify_Headers ()
{
  int i, ok, last;
  ok = 1;

return(ok);

  last = (doubledisk) ? NPAGES * 2 : NPAGES;
  for (i = 0; i < last; i += 1)
    ok &= Assert (disk[i].pagenum == RDAtoVDA (disk[i].header[1]),
		  "page %04x header doesn't match: %04x %04x\n",
		  disk[i].pagenum, disk[i].header[0], disk[i].header[1]);
  return (ok);
}

int
ValidateDiskDescriptor ()
{
  /*
   * check numdisks
   *
   */
  int ddlp, i, page, free, ok, last;
  struct LEADER *lp;
  struct LABEL *l;
  struct FA fa;
  /* locate DiskDescriptor and copy it into the global data structure */
  ddlp = find_file ("DiskDescriptor");
  ok = Assert (ddlp != -1, "Can't find DiskDescriptor\n");
  if (!ok)
    return (ok);
  lp = pageLeader (ddlp);
  l = pageLabel (ddlp);
  fa.vda = RDAtoVDA (l->nextRDA);
  bcopy (&disk[fa.vda].data[0], &kdh, sizeof (kdh));
  bitTable = (word *) malloc (kdh.diskBTsize * sizeof (word));
  /* now copy the bit table from the disk into bitTable */
  fa.pageNumber = 1;
  fa.charPos = sizeof (kdh);
  for (i = 0; i < kdh.diskBTsize; i += 1)
    bitTable[i] = getword (&fa);
  /* for single disk systems, (only one supported now) */
  ok &= Assert (kdh.nDisks == 1, "only support single disk systems\n");
  ok &= Assert (kdh.nTracks == 203, "KDH tracks != 203\n");
  ok &= Assert (kdh.nHeads == 2, "KDH heads != 2\n");
  ok &= Assert (kdh.nSectors == 12, "KDH sectors != 12\n");
  ok &= Assert (kdh.defVersionsKept == 0, "defaultVersions != 0\n");
  /* count free pages in bit table */
  free = 0;
  last = (doubledisk) ? NPAGES * 2 : NPAGES;
  for (page = 0; page < last; page += 1)
    free += (getBT (page) == 0);
  ok &= Assert (free == kdh.freePages,
		"bit table count %d doesn't match KDH free pages %d\n",
		free, kdh.freePages);
  /* count free pages in actual image */
  free = 0;
  for (page = 0; page < last; page += 1)
    free += pagefree (page);
  ok &= Assert ((free == kdh.freePages),
		"actual free page count %d doesn't match KDH value %d\n",
		free, kdh.freePages);
  return (ok);
}

void
FixDiskDescriptor ()
{
  int page, free, t, last;
  /* rebuild bit table and free page count from labels */
  free = 0;
  last = (doubledisk) ? NPAGES * 2 : NPAGES;
  for (page = 0; page < last; page += 1)
    {
      t = pagefree (page);
      free += t;
      setBT (page, t ^ 1);
    }
  kdh.freePages = free;
}

/****************************/
/* general support routines */
/****************************/
int
Assert (int bool, char *errmsg, ...)
{
  va_list ap;
  if (!bool)
    {
      va_start (ap, errmsg);
      vprintf (errmsg, ap);
      va_end (ap);
    }
  return (bool);
}

void
AssertOrDie (int bool, char *errmsg, ...)
{
  va_list ap;
  if (!bool)
    {
      va_start (ap, errmsg);
      vprintf (errmsg, ap);
      va_end (ap);
      exit (1);
    }
}

void
swabit (char *data, int count)
{
  word junk, *d;
  AssertOrDie (((count & 1) == 0) && (((long) data & 1) == 0),
	       "swab called with unaligned values\n");
  count >>= 1;
  d = (word *) data;
  while (count--)
    {
      junk = *d;
      junk = ((junk >> 8) & 0xff) | (junk << 8);
      *d++ = junk;
    }
}

void
copystring (char *to, char *from, int length, int lower)
{
  int i;
  char c;
  for (i = 0; i < length; i += 1)
    {
      if (little.l)
	c = from[i ^ 1];
      else
	c = from[i];
      if (lower)
	c = tolower ((unsigned char)c);
      to[i] = c;
    }
}


