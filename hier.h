/* hier.h */

#ifndef HIER_H
#define HIER_H

#define HEADINGSIZE 	256  /* max. size of a heading's text */
#define FSUFFIXSIZE 	8    /* max. number of chars to use as file suffix */

#ifndef DEFAULT_LANG
#define DEFAULT_LANG	rtfLangGerman
#endif

#define NAV_NUMBER	11
#define NAV_PREVIOUS	0
#define NAV_NEXT	1
#define NAV_UP		2
#define NAV_TITLE	3
#define NAV_CONTENTS	4
#define NAV_INDEX	5
#define NAV_DELIM	6   /* only used by customized navigation panels */
#define NAV_NOPREVIOUS	7   /* so are these */
#define NAV_NONEXT	8
#define NAV_NOUP	9
#define NAV_NOTITLE	10

#define PREVIOUS_TEXT	"&lt;&lt;"
#define NEXT_TEXT	"&gt;&gt;"
#define UP_TEXT		"up"
#define TITLE_TEXT	"Title"
#define CONTENTS_TEXT 	"Contents"
#define INDEX_TEXT 	"Index"

#define FRE_PREVIOUS_TEXT	"&lt;&lt;"
#define FRE_NEXT_TEXT		"&gt;&gt;"
#define FRE_UP_TEXT		"Remonter"
#define FRE_TITLE_TEXT		"Titre"
#define FRE_CONTENTS_TEXT 	"Sommaire"
#define FRE_INDEX_TEXT 		"Index"

#define GER_PREVIOUS_TEXT	"&lt;&lt;"
#define GER_NEXT_TEXT		"&gt;&gt;"
#define GER_UP_TEXT		"hoch"
#define GER_TITLE_TEXT		"Titelseite"
#define GER_CONTENTS_TEXT 	"Inhaltsverzeichnis"
#define GER_INDEX_TEXT 		"Index"

#define ITA_PREVIOUS_TEXT	"&lt;&lt;"
#define ITA_NEXT_TEXT		"&gt;&gt;"
#define ITA_UP_TEXT		"Risalire"
#define ITA_TITLE_TEXT		"Titolo"
#define ITA_CONTENTS_TEXT 	"Sommario"
#define ITA_INDEX_TEXT 		"Indice"

#define CSP_PREVIOUS_TEXT       "&lt;&lt;"
#define CSP_NEXT_TEXT           "&gt;&gt;"
#define CSP_UP_TEXT             "Anterior"
#define CSP_TITLE_TEXT          "Título"
#define CSP_CONTENTS_TEXT       "Contenido"
#define CSP_INDEX_TEXT          "Indice"

typedef struct _hierHeading {
   struct _hierHeading *next;	/* pointer to next heading */
   struct _hierHeading *peer;	/* pointer to next heading with same level */
   struct _hierHeading *father;	/* pointer to father heading */
   struct _hierHeading *fchild;	/* pointer to next heading with level + 1*/

   int level;			/* level of this heading (1 .. 6) */
   int number;			/* running number of this heading */
   int filenum;			/* file number containing this heading */
   int index;			/* 1 if this heading contains index entries */
   char text[HEADINGSIZE+2];	/* text of this heading */
   char *start;			/* Start tag for this heading number */
   char *end;			/* End tag for this heading number */
} HierHeading;

typedef struct _hierFile {
   struct _hierFile *next;		/* pointer to next file */
   struct _hierHeading *heading;	/* pointer to heading of this file */
   char fsuffix[FSUFFIXSIZE + 4];	/* file suffix for this heading */
} HierFile;

typedef struct _hierIndex {
   struct _hierIndex *next;	/* pointer to next index entry */
   struct _hierHeading *heading;	/* if index in head., points to this */
   int number;			/* running number of this entry */
   int filenum;			/* file number containing this entry */
   int level;			/* level of index entry (for collapsing) */
   char *textp;                 /* points to start of visible text */
   char text[HEADINGSIZE+2];		/* text of entry */
} HierIndex;

typedef struct _hierAnchor {
    struct _hierAnchor *next;		/* pointer to next anchor */
    struct _hierHeading *heading;	/* pointer to heading of this anchor */
    char text[HEADINGSIZE+2];             /* text of this anchor */
} HierAnchor;

extern int hierPass;		/* 1 if -h-option was given (hierarchical) */
extern int hierDepth;		/* optional parameter to -h-option */
extern int inHeading;		/* 1 if reader inside a heading */
extern int inHeadingRef;	/* 1 if reader inside a heading reference */
extern int groupInHeading;	/* 1 if reader inside a group inside heading */
extern int groupInHeadingCount;
extern int inIndexEntry;	/* 1 if reader inside an index entry */
extern int hierHasText;		/* 1 if section contains "normal" text */
extern int hierContents;	/* 1 if -c-option was given (toc) */
extern int hierRefsOnTop;	/* 1 if all heading-refs should appear on top */
extern int hierIndex;		/* 1 if -x-option was given (index) */
extern int hierShortFileNames;	/* 1 if -s option was given (short fnames) */
extern int hierUseFrames;	/* 1 if -F option was given (use frames) */
extern int hierIsChangingFile;	/* 1 if HTMLCleanup is called from hier.c */
extern char headingRef[];	/* current heading reference */
extern char indexEntry[];	/* current index entry */
extern FILE *hierFile;		/* current output-FILE */
extern char anchorBuf[];        /* buffer of href text */

char *hierUsageString ();
char *hierCreationString ();    /* returns the file creation string */
void doHierPass ();		/* starts the preprocessing pass */
int hierHeadingLevel();		/* returns level if heading, else 0 */
char *hierOutFile ();		/* returns a new filename */
void hierProlog ();		/* Write a prolog to file outfd */
void hierEpilog ();		/* Write an epilog to file outfd */
void hierChangeOutFile ();	/* change output file if necessary */
int colorIsRed ();		/* see if color colnum is "red" */
void hierResolve ();		/* tries to resolve headingRef */
HierIndex *addIndexEntry ();	/* add indexEntry to index-list */
void generateIndex ();		/* create a file for the index */
void hierSetDocTitle ();	/* set the document title */
void hierSetIndexAnchor ();     /* set text of index anchors */
void hierSetNavPanel ();        /* load navigation panel config-file */
char *FileOfAnchor ();          /* name of html file anchor has been def. in */
char *hierCutShort ();          /* cuts basename of file to a max. of 6 char */

/* from htmlout.c: */
int HTMLInit ();
int HTMLCleanup ();

#endif /* HIER_H */







