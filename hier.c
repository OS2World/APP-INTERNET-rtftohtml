/*
 *	hier.c - Module to implement hierarchical structuring into rtftohtml.
 *
 *	Author: 	Christian Bolik (zzhibol@rrzn-user.uni-hannover.de)
 *
 *			(c) 1994-96 by Christian Bolik
 *			May be freely distributed.
 *
 *	Started: 	05.05.94
 *	Last changed:	07.02.96
 *	Version:	1.7
 *
 *	ChangeLog:
 *			06.07.94 updated to work with rtftohtml-2.7 --> V1.1
 *			21.07.94 no more ':' in filenames (newHierFile),
 *				 new title for html-documents
 *			25.07.94 improved -h0 behaviour, bug fixes --> V1.2
 *			19.08.94 made rtftoweb avoid '/' and '+' in filenames
 *			26.09.94 check for RTFGetStyle() == NULL,
 *				 removed bug in hierIntList,
 *				 several minor changes,
 *				 added -T cmdline option,
 *				 works with rtftohtml 2.7.3 --> V1.3
 *			29.09.94 use html-trans for style identification
 *				 works with rtftohtml 2.7.4 --> V1.4
 *			21.11.94 removed STUPID bug in hierHeadingLevel that
 *				 that produced frequent seg-faults,
 *				 added support for french and italian langs
 *				 (contributed by achille@aisws5.cern.ch),
 *				 works with rtftohtml 2.7.5 --> V1.5,
 *				 minor changes
 *                      01.08.95 For a list of changes since november '94
 *                               have a look at the CHANGES file.
 *
 */

#ifdef CB_ADD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
# define Edef
#include "rtf.h"
#include "rtftohtml.h"
#include "hier.h"

#define NAV_CONFIG_FILE "nav-panel"
#define ISIDEL(c) (((c) == ' ' || (c) == ',' || (c) == ':') ? 1 : 0)

char rtftowebVersion[6] = "1.7";

int hierPass = 0;
int inHeading = 0;
int inHeadingRef = 0;
int groupInHeading = 0;
int groupInHeadingCount = 0;
int inIndexEntry = 0;
int hierDepth = 9999;	/* this means: split to maximum depth */
int hierHasText = 0;
int hierContents = 0;
int hierRefsOnTop = 0;
int hierIndex = 0;
int hierShortFileNames = 0;
int hierUseFrames = 0;
int hierIsChangingFile = 0;

char headingRef[HEADINGSIZE];
char indexEntry[HEADINGSIZE];
char anchorBuf[256];

FILE *hierFile = NULL;

static HierHeading *hierList = NULL;
static HierFile *hierFileList = NULL;
static HierIndex *hierIndexList = NULL;
static HierAnchor *hierAnchorList = NULL;

static void hierControl (/*void*/);
static void hierText (/*void*/);
static void hierGroup (/*void*/);
static HierHeading *newHeading (/*void*/);
static void hierProcess (/*void*/);
static HierFile *newHierFile (/*HierHeading *heading*/);
static char *fsuffix (/*int filenum*/);
static HierAnchor *newHierAnchor ();

static void hierExtList (/*HierHeading *hh, FILE *outfd*/);
static HierHeading *hierIntList (/*HierHeading *hh, FILE *outfd*/);
static void generateContents (/*int framed*/);
static HierHeading *hierContList (/*HierHeading *hh, FILE *outfd*/);

static void setNavTexts (/*int lang*/);
static int hierNavPanel (/*FILE *outfd, int framed*/);

static HierIndex *newIndexEntry (/*void*/);
static HierIndex *sortIndex (/*void*/);
static HierIndex *preprocessIndex (/*void*/);
static int matchOffset (/*char *s1, s2*/);
static char *trimleft (/*char *str, char *trim*/);
static void generateFrames (/*void*/);
static int alpha_strcmp (/*char *s1, char *s2*/);
static int alpha_strncmp (/*char *s1, char *s2*, int n*/);
static char *str_replace (/*char *str, char *this, char *that*/);

static int inTitle = 0;
static int indexInHeading = 0;
static hiddenText = 0;
static allcapsText = 0;
static optDest = 0;
static HierHeading *currentHH = NULL;
static char docTitle[256] = "";
static char navText[NAV_NUMBER][256];
static char indexAnchorText[32] = "";
static char titleContents[256];
static char titleIndex[256];
static char horiLine[256] = "<HR>";
static char bodyBgImage[256] = "";
static char bodyBgColor[16] = "";
static char bodyTextColor[16] = "";
static char bodyLinkColor[16] = "";
static char bodyVLinkColor[16] = "";
static char bodyALinkColor[16] = "";
static char HtmlExt[6] = "html";
static char CSuffix[16] = "-Contents";  /* suffix for the contents file */
static char ISuffix[16] = "-Index";     /* suffix for the index file */
/* these will additionally be used if the -F option was given (use frames): */
static char TSuffix[16] = "-Title";     /* suffix for the title file */
static char FCSuffix[16] = "-FContents";  /* suffix for the contents file */
static char FISuffix[16] = "-FIndex";     /* suffix for the index file */

static int num_headings = 0;
static int num_files = 0;
static int num_index_entries = 0;

/* Field to check for empty headings, heading i empty --> hhValid[i] = 0. */
static char *hhValid;
static int numHeadings = 0;

static int file_number = -1;	/* number of opened files - 1 */
static int heading_number = 0;	/* number of read headings */

static int customNavPanel = 0;
static int hcharcount = 0;
static int first_pass = 0;

static int inParNum = 0;
static char parNumBuf[128];

static int inMTName = 0;
static HierAnchor *currentHA = NULL;


extern int IStyle_Chg;
extern TStyle_typ TStyle;
extern char *TFont;
extern int TSize;
extern void CharAttr ();


/* ========================================================================= */

char *hierUsageString ()
{
    static char usage[BUFSIZ];

    strcpy (usage, "Usage: rtftohtml [options] file\n");
    sprintf (usage, "%s(Info: This is rtftohtml %s with applied rtftoweb %s patches.)\n\n", usage, PVers, rtftowebVersion);
    strcat (usage, "Options may be:\n");
    strcat (usage, "  -c        generate a Table of Contents page (req. -h)\n");
    strcat (usage, "  -F        use frames (req. -c and Netscape 2.0 or compatible)\n");
    strcat (usage, "  -G        write no graphics files\n");
    strcat (usage, "  -h[n]     split output at headings (at the n'th level)\n");
    strcat (usage, "  -i        imbed graphics\n");
    strcat (usage, "  -N file   read navigation panel from \"file\" (req. -h)\n");
    strcat (usage, "  -o file   write output to \"file\"\n");
    strcat (usage, "  -P ext    use \"ext\" as extension for external graphic files\n");
    strcat (usage, "  -s        use short filenames when splitting\n");
    strcat (usage, "  -t        list external references on top of page (req. -h)\n");
    strcat (usage, "  -T title  use \"title\" as the document title (req. -h)\n");
    strcat (usage, "  -V        print version nuber of rtftohtml (only)\n");
    strcat (usage, "  -x        create an Index (if there are index entries) (req. -h)\n");
    strcat (usage, "  -X text   use \"text\" as the text for index anchors (e.g. &#183;)\n");

    return (usage);
} /* hierUsageString */


/* ------------------------------------------------------------------------- */

char *hierCreationString ()
{
    static char str[160];

    sprintf (str, "<!-- This document was created from RTF source by rtftohtml version %s,\n     extended by rtftoweb version %s. -->\n", 
	     PVers, rtftowebVersion);

    return (str);
} /* hierCreationString */

/* ------------------------------------------------------------------------- */

void doHierPass ()
{
    /* the RTF-file has already been opened */
    /* Now prepare for the preprocessing pass: */
    first_pass = 1;
   
    /* install class callbacks */
    RTFSetClassCallback (rtfControl, hierControl);
    RTFSetClassCallback (rtfText, hierText);
    RTFSetClassCallback (rtfGroup, hierGroup);

    /* de-install default destination callbacks */
    RTFSetDestinationCallback (rtfInfo, NULL);
    RTFSetDestinationCallback (rtfPict, NULL);
    RTFSetDestinationCallback (rtfObject, NULL);

    setNavTexts (DEFAULT_LANG);
    if (hierShortFileNames) {
	strcpy (HtmlExt, "htm");
	strcpy (CSuffix, "_c");
	strcpy (ISuffix, "_i");
	strcpy (TSuffix, "_t");
	strcpy (FCSuffix, "fc");
	strcpy (FISuffix, "fi");
    }

   /* now preprocess the input */
    RTFRead ();
    if (!strlen(docTitle)) {
	strcat (docTitle, "Without a title");
    }
#  ifdef DEBUG
    fprintf(stderr, "docTitle: %s\n", docTitle);
#  endif

    /* fill required data structures */
    hierProcess();

    /* clean things up */
    first_pass = 0;
    inHeading = 0;
    hierPass = 2;
    rewind (stdin);
    RTFInit ();
} /* doHierPass */

/* ------------------------------------------------------------------------- */

static void hierControl ()
{
    int level;
    static int last_level = 0;
    static HierHeading *last_hh = NULL;

    switch (rtfMajor) {

    case rtfParAttr:
	/* Paragraph Style Token */
	switch (rtfMinor) {
	case rtfParDef:
	    TStyle = stylePlain;
	    if (inHeading && !groupInHeading) {
		inHeading = 0;
#           ifdef DEBUGW
		fprintf(stderr, "%s\n", currentHH->text);
		fprintf(stderr, "Length: %d\n", strlen (currentHH->text));
		fprintf(stderr, "rtfParDef encountered\n");
#           endif
	    }
	    break;
	case rtfStyleNum: {
	    char *start, *end;
	    
	    if ((level = hierHeadingLevel (rtfParam, &start, &end))
		&& !inHeading) {
		inHeading = 1;
		hcharcount = 0;
		currentHH = newHeading ();
		if (currentHH == hierList) {
		    /* Force first heading to be regarded as a level 1
		       heading, to avoid strange errors if the first heading
		       is at another level. */
		    currentHH->level = 1;
		    last_level = 1;
		} else {
		    currentHH->level = level;
		    if (level - last_level >= 2)
			fprintf (stderr, "Warning: level %d heading followed"
				 " by level %d heading\n  (at \"%s\")\n",
				 last_level, level, last_hh->text);
		    last_level = level;
		}
 		currentHH->start = start;
 		currentHH->end = end;
		last_hh = currentHH;
#        ifdef DEBUG
		fprintf(stderr, "rtfStyleNum: %d = %s, Level %d\n", rtfParam, 
			RTFGetStyle(rtfParam)->rtfSName, currentHH->level);
#        endif
	    }
	    break;
	}
	}
	break;

    case rtfCharAttr:
	/* look for language of RTF-file */
	switch (rtfMinor) {
	case rtfPlain:
	    hiddenText = 0;
	    allcapsText = 0;
	    break;
	case rtfInvisible:
	    hiddenText = (rtfParam ? 1 : 0);
	    break;
	case rtfLanguage:
	    setNavTexts (rtfParam);
	    break;
        case rtfSmallCaps:
        case rtfAllCaps:
	    allcapsText = (rtfParam ? 1 : 0);
	}
	CharAttr ();
	break;

    case rtfSpecialChar:
	/* convert special characters in headings (e.g. to spaces) */
	if (inHeading || inParNum) {
	    switch (rtfMinor) {
	    case rtfLine:
		if (!inParNum)
		    strcat (currentHH->text, "<BR>");
		else
		    strcat (parNumBuf, " ");
		break;
	    case rtfTab:
	    case rtfNoBrkSpace:
		if (!inParNum)
		    strcat (currentHH->text, " ");
		else
		    strcat (parNumBuf, " ");
		break;
	    case rtfOptDest:
		optDest = 2;
		break;
	    }
	}
	break;

    case rtfDestination:
	if (optDest)
	    optDest--;

	/*if (inHeading) {
	  fprintf (stderr, "destination: %d\n", rtfMinor);
	  }*/
	switch (rtfMinor) {

	case rtfIndex:
	    if (hierIndex && inHeading) 
		/* catch index entries that appear in headings */
		indexInHeading = 1;
	    break;
	case rtfITitle:
	    if (*docTitle == 0) {
		/* no title yet */
		inTitle = 1;
#           ifdef DEBUG
		fprintf(stderr, "inTitle = 1\n");
#           endif
	    }
	    break;
	case rtfParNumText:
	    inHeading = 0;
	    inParNum = 1;
	    *parNumBuf = 0;
	    break;
	case rtfParNumbering:
	case rtfParNumTextAfter:
	case rtfParNumTextBefore:
	    hiddenText = 1;
	    /* no break */
       
	default:
	    inParNum = 0;
	    inTitle = 0;

	}
	break;
    }
} /* hierControl */

/* ------------------------------------------------------------------------- */

static char *handleAllCaps (str)
char *str;
/* Handles the All/SmallCaps attribute. Merci a Daniel! */
{
    char *s = str;
    
    if (allcapsText && str[0] != '&') {
	while (*s) {
	    if (islower (*s))
		*s = toupper (*s);
	    s++;
	}
    }

    return str;
} /* handleAllCaps */

/* ------------------------------------------------------------------------- */

static void hierText ()
{
    int i, mcharlen;
    char *mchar = MapChar (rtfMinor);

    mcharlen = strlen (mchar);
    handleAllCaps (mchar);

    if (IStyle_Chg) {
	/* try to catch text for the MTName special */
	IStyle_Chg = 0;
	for (i = 0; i < TMatchLen; i++) {
	    if ((TMatchArr[i].TStyle == (TMatchArr[i].TMask & TStyle)) &&
		(TMatchArr[i].Font[0] == '\0' ||
		 strcmp(TMatchArr[i].Font, TFont) == 0) &&
		(TMatchArr[i].FSize == 0 || TMatchArr[i].FSize == TSize)) {
		if (TMatchArr[i].TTidx == MTName) {
		    inMTName = 1;
		    currentHA = newHierAnchor ();
		    strcpy (currentHA->text, mchar);
		    return;
		} else if (inMTName) {
		    inMTName = 0;
		}
		break;
	    }
	}
	if (i == TMatchLen && inMTName) {
	    inMTName = 0;
	}
    }

    if (inHeading) {
	if (hcharcount < HEADINGSIZE - mcharlen) {
	    if (!hiddenText && !optDest) {
		hcharcount += mcharlen;
		strcat (currentHH->text, mchar);
	    } else if (indexInHeading)
		strcat (indexEntry, mchar);
	} else {
	    fprintf (stderr, "Warning: encountered a heading longer then %d chars, ignoring it\n", HEADINGSIZE);
	    *currentHH->text = 0;
	    inHeading = 0;
	}
    } else if (inParNum) {
	if (rtfMinor != rtfSC_nothing) {
	    /* this needs to be fixed sometimes... */
	    strcat (parNumBuf, mchar);
	}
    } else if (inTitle) {
	strcat (docTitle, mchar);
    } else if (inMTName) {
	strcat (currentHA->text, mchar);
    }

} /* hierText */

/* ------------------------------------------------------------------------- */

static void hierGroup ()
{
    static int groupInHeadingCount = 0, 
	groupInIndexCount = 0,
	hiddenTextCount = 0,
	allcapsTextCount = 0,
	inParNumCount = 0;

    switch (rtfMajor) {
    case rtfBeginGroup:
	if (inHeading) {
	    groupInHeading = 1;
	    groupInHeadingCount++;
	}
	if (indexInHeading)
	    groupInIndexCount++;
	if (hiddenText)
	    hiddenTextCount++;
	if (allcapsText)
	    allcapsTextCount++;
	if (inParNum)
	    inParNumCount++;
	break;
    case rtfEndGroup:
	if (groupInHeadingCount) {
	    groupInHeadingCount--;
	}
	if (groupInHeadingCount == 0 && groupInHeading)
	    groupInHeading = 0;
	if (indexInHeading) {
	    if (groupInIndexCount)
		groupInIndexCount--;
	    else {
		indexInHeading = 0;
		addIndexEntry()->heading = currentHH;
		currentHH->index = 1;
	    }
	}
	if (hiddenText) {
	    if (hiddenTextCount)
		hiddenTextCount--;
	    else
		hiddenText = 0;
	}
	if (allcapsText) {
	    if (allcapsTextCount)
		allcapsTextCount--;
	    else
		allcapsText = 0;
	}
	if (inParNum) {
	    if (inParNumCount)
		inParNumCount--;
	    else
		inParNum = 0;
	}
	if (inMTName) {
	    TStyle = stylePlain;
	    inMTName = 0;
#ifdef DEBUG
	    fprintf (stderr, "MTName: %s, heading: %s\n", currentHA->text,
		     (currentHA->heading) ? currentHA->heading->text : "");
#endif
	}
	optDest = 0;
	break;
    }
} /* hierGroup */

/* ------------------------------------------------------------------------- */

extern int parnest;

int hierHeadingLevel(style_num, start, end)
int style_num;
char **start, **end;
/* Returns the level of style style_num if it's a heading, otherwise 0 */
{
    char style_name[64];
    int level = 0, i, ptag;
    RTFStyle *styleptr;
    static int headingcnt = 0;

    if (parnest > 0)
	/* We don't want to deal with styles the current
	   one is based on: */
	return 0;

    styleptr = RTFGetStyle(style_num);
    if (styleptr == (RTFStyle *) NULL)
	return (0);

    strcpy(style_name, styleptr->rtfSName);

    /*
     * Check if the PTag for this style is a heading tag:
     */

    for (i = 0; i < PMatchLen; i++) {
	if (strcmp (style_name, PMatchArr[i].PStyle) == 0) {

	    if (PMatchArr[i].PTidx >= MTSPECIAL)
		continue;

 	    ptag = PMatchArr[i].PTidx;
 	    level = PTagArr[ptag].ToCLev;
 	    if (start)
 		*start = PTagArr[ptag].StartTag;
 	    if (end)
 		*end = PTagArr[ptag].EndTag;
 
 	    if (level > 0 && !first_pass && !inHeading) {
 		if (!hhValid[headingcnt++])
 		    level = 0;
 	    }
 	    
 	    return level;
	}
    }

    return (0);
} /* hierHeadingLevel */

/* ------------------------------------------------------------------------- */

static HierHeading *newHeading ()
{
    HierHeading *newhh, *hh;

    newhh = (HierHeading *) RTFAlloc (sizeof (HierHeading));
#ifdef DEBUGM
    fprintf (stderr, "RTFAlloc in newHeading returns: %p\n", newhh);
#endif
    if (!newhh)
	RTFPanic("Not enough memory in newHeading!\n");

    newhh->next = newhh->peer = newhh->father = newhh->fchild = NULL;
    newhh->index = 0;
    newhh->level = 1;
    if (!inParNum) {
	strcpy(newhh->text, "");
    } else {
	inParNum = 0;
	strcpy(newhh->text, parNumBuf);
    }       

    hh = hierList;
    if (hh == NULL)
	hierList = newhh;
    else {
	while (hh->next)
	    hh = hh->next;
	hh->next = newhh;
    }

    numHeadings++;
    
    return (newhh);
} /* HierHeading */

/* ------------------------------------------------------------------------- */

static void hierProcess ()
/* processes the filtered hierarchy */
{
    HierHeading *hh = hierList, *ihh, *jhh, *phh = NULL;
    HierIndex *hi;
    int i = 0;

    if (numHeadings == 0) return;
    
    hhValid = (char *) RTFAlloc (numHeadings * sizeof(char));
    if (!hhValid)
	RTFPanic ("Out of memory in hierProcess.\n");

    /* remove empty headings: */
    while (hh) {
	if (!*(hh->text)) {
	    if (!phh)
		hierList = hh->next;
	    else
		phh->next = hh->next;

	    ihh = hh;
	    hh = hh->next;
	    RTFFree (ihh);
	    hhValid[i] = 0;
	} else {
	    phh = hh;
	    hh = hh->next;
	    hhValid[i] = 1;
	}
	i++;
    }

    hh = hierList;
    while (hh) {
	ihh = hh->next;

	/* find fchild and peers */

	while (ihh) {
	    if (!hh->fchild && ihh->level > hh->level) {
		hh->fchild = ihh;
		ihh->father = hh;
		jhh = ihh->next;
		while (jhh) {
		    if (jhh->level == ihh->level)
		   	jhh->father = ihh->father;

		    if (jhh->level < ihh->level)
			break;
		    jhh = jhh->next;
		}
	    }
	    if (ihh->level == hh->level) {
		hh->peer = ihh;
		break;
	    }
	    if (ihh->level < hh->level)
		break;

	    ihh = ihh->next;
	}

	num_headings++;
	hh->number = num_headings;
	hh = hh->next;	
    }

    num_files = 1;
    hh = hierList;
    while (hh) {
	if (hh->level <= hierDepth) {
	    newHierFile (hh);
	    num_files++;
	}
	hh->filenum = num_files - 1;
	hh = hh->next;
    }
#  ifdef DEBUG
    fprintf (stderr, "num_headings: %d, num_files: %d\n", 
	     num_headings, num_files);
#  endif

    if (hierIndexList) {
	/* oh, there have been indexes in headings, postprocess them */
	hi = hierIndexList;
	while (hi) {
	    if (hi->heading) {
		hi->filenum = hi->heading->filenum;
	    }
	    hi = hi->next;
	}
    }

} /* hierProcess */

/* ------------------------------------------------------------------------- */

static HierFile *newHierFile (heading)
HierHeading *heading;
{
    HierFile *newhf, *hf;
    char word1[FSUFFIXSIZE + 4], *fs;
    int used;

    newhf = (HierFile *) RTFAlloc (sizeof (HierFile));
#ifdef DEBUGM
    fprintf (stderr, "RTFAlloc in newHierFile returns: %p\n", newhf);
#endif
    if (!newhf)
	RTFPanic("Not enough memory in newHierFile!\n");

    newhf->next = NULL;
    newhf->heading = heading;

    if (hierShortFileNames) {
	sprintf (newhf->fsuffix, "%d", num_files);
	goto fsuffix_set;  /* YES! */
    }
	

    /* find a descriptive suffix for this file's name */
    word1[FSUFFIXSIZE] = 0;
    strncpy (word1, heading->text, FSUFFIXSIZE);
    if (strlen (word1) == FSUFFIXSIZE) {
	/* find first space and cut it off */
	fs = strchr (word1, ' ');
	if (fs)
	    *fs = 0;
    } /* else {
	 fs = word1;
	 while (*fs) {
	 if (*fs == ' ')
	 *fs = '_';
	 fs++;
	 }
	 } */

    /* replace critical characters with underlines */
    fs = word1;
    while (*fs) {
	if (*fs == ' ')
	    *fs = '_';
	else if (*fs == ':')	/* Mosaic doesn't like colons in filenames */
	    *fs = '_';
	else if (*fs == '+')	/* and no plus */
	    *fs = '_';
	else if (*fs == '/')	/* and noone likes slashes */
	    *fs = '_';
	else if (*fs == '&')	/* Well, what does it like, anyway ?! */
	    *fs = '_';
	else if (*fs == ';')
	    *fs = '_';
	else if (*fs == '?')
	    *fs = '_';
	else if (*fs == '"')
	    *fs = '_';
	else if (*fs == '\'')
	    *fs = '_';
	else if (*fs == '`')
	    *fs = '_';
	fs++;
    }

    /* remove a trailing '.', looks better */
    /* after the while loop fs points to the trailing NUL */
    if (*(fs - 1) == '.')
	*(fs - 1) = 0;

    sprintf (newhf->fsuffix, "-%s", word1);
   
    /* now see if this suffix is already used */
    if (!strcmp (newhf->fsuffix, CSuffix))
	strcat (newhf->fsuffix, "-2");
    if (!strcmp (newhf->fsuffix, ISuffix))
	strcat (newhf->fsuffix, "-2");
    if (hierUseFrames) {
	if (!strcmp (newhf->fsuffix, TSuffix))
	    strcat (newhf->fsuffix, "-2");
	if (!strcmp (newhf->fsuffix, FCSuffix))
	    strcat (newhf->fsuffix, "-2");
	if (!strcmp (newhf->fsuffix, FISuffix))
	    strcat (newhf->fsuffix, "-2");
    }
    used = 1;
    hf = hierFileList;
    while (hf) {
	while (hf) {
	    if (!strcmp (newhf->fsuffix, hf->fsuffix)) {
		used++;
		sprintf (newhf->fsuffix, "-%s-%d", word1, used);
		hf = hierFileList;
		break;
	    }
	    hf = hf->next;
	}
    }

    fsuffix_set:
    hf = hierFileList;
    if (hf == NULL) {
	hierFileList = newhf;
    } else {
	while (hf->next)
	    hf = hf->next;
	hf->next = newhf;
    }

    return (newhf);
}

/* ------------------------------------------------------------------------- */

static char *fsuffix (filenum)
int filenum;
/* returns fsuffix for a given file-number */
{
    HierFile *hf = hierFileList;
    int i;

    if (filenum == 0) {
	if (hierUseFrames)
	    return (TSuffix);
	else
	    return ("");
    }

    for (i = 1; i < filenum; i++)
	hf = hf->next;

    return (hf->fsuffix);
} /* fsuffix */

/* ========================================================================= */

char *hierOutFile ()
{
    HierFile *hf = hierFileList;
    static char fname[128];
    int i;

    file_number++;

    if (file_number > 0) {
	if (file_number == 1 && hierUseFrames)
	    generateFrames ();
	
	for (i = 1; i < file_number; i++)
	    hf = hf->next;
	sprintf(fname, "%s%s.%s", FPrefix, hf->fsuffix, HtmlExt);
    } else {
	sprintf(fname, "%s%s.%s", FPrefix, (hierUseFrames ? TSuffix : ""),
		HtmlExt);
    }

#  ifdef DEBUG
    fprintf (stderr, "hierOutFile: %s\n", fname);
#  endif

    return (fname);
} /* hierOutFile */

/* ------------------------------------------------------------------------- */

static HierAnchor *newHierAnchor ()
{
    HierAnchor *newha, *ha;

    newha = (HierAnchor *) RTFAlloc (sizeof (HierAnchor));
#ifdef DEBUGM
    fprintf (stderr, "RTFAlloc in newHierAnchor returns: %p\n", newha);
#endif
    if (!newha)
	RTFPanic("Not enough memory in newHierAnchor!\n");

    newha->next = NULL;
    newha->heading = currentHH;
    strcpy (newha->text, "");

    ha = hierAnchorList;
    if (ha == NULL) {
	hierAnchorList = newha;
    } else {
	while (ha->next)
	    ha = ha->next;
	ha->next = newha;
    }

    return (newha);
} /* newHierAnchor */

char *FileOfAnchor (anchor)
char *anchor;
/* returns the name of the html-file in which anchor has been defined
 or "". anchor contains the complete href. */
{
    HierAnchor *ha;
    static char fname[256];
    char buf[256];
    
    strcpy (fname, "");
    if (anchor[0] != '#') {
	return (fname);
    } else {
	strcpy (buf, anchor+1);
    }

    ha = hierAnchorList;
    while (ha) {
	if (!strcmp (ha->text, buf))
	    break;
	ha = ha->next;
    }

    if (!ha) {
	fprintf (stderr, "%s: no such anchor resp. _Name\n", buf);
    } else {
	if (ha->heading) {
	    if (ha->heading->filenum != file_number)
		sprintf (fname, "%s%s.%s", FPrefix,
			 fsuffix (ha->heading->filenum), HtmlExt);
	} else {
	    /* anchor is located in the title page */
	    if (file_number > 0)
		sprintf (fname, "%s.%s", FPrefix, HtmlExt);
	    /* else the anchor is referenced from the title page itself */
	}
    }
	
    return (fname);
} /* FileOfAnchor */

/* ------------------------------------------------------------------------- */

char *bodyTag ()
/* creates a <body> tag with options as specified via navpanel customization */
{
    static char buf[1024];
    
    strcpy (buf, "\n<BODY");
    if (*bodyBgImage)
    	sprintf (buf, "%s BACKGROUND=%s", buf, bodyBgImage);
    if (*bodyBgColor)
    	sprintf (buf, "%s BGCOLOR=%s", buf, bodyBgColor);
    if (*bodyTextColor)
    	sprintf (buf, "%s TEXT=%s", buf, bodyTextColor);
    if (*bodyLinkColor)
    	sprintf (buf, "%s LINK=%s", buf, bodyLinkColor);
    if (*bodyVLinkColor)
    	sprintf (buf, "%s VLINK=%s", buf, bodyVLinkColor);
    if (*bodyALinkColor)
    	sprintf (buf, "%s ALINK=%s", buf, bodyALinkColor);
    strcat (buf, ">");
    
    return (buf);
} /* bodyTag */

void hierProlog (outfd)
FILE *outfd;
/* Write a prolog to file outfd with number file_number. */
{
    HierHeading *hh;
    HierFile *hf = hierFileList;
    int i;

    fprintf (outfd, "<HTML><HEAD>\n");
    fprintf (outfd, "%s", hierCreationString ());
    if (file_number == 0) {
	fprintf (outfd, "<TITLE>%s - Title</TITLE></HEAD>%s\n", 
	         docTitle, bodyTag ());
    } else {
	for (i = 1; i < file_number; i++)
	    hf = hf->next;
	fprintf (outfd, "<TITLE>%s - %s</TITLE></HEAD>%s\n", 
		 docTitle, str_replace(hf->heading->text, "<BR>", " "),
		 bodyTag ());
    }

    if (hierNavPanel (outfd, 0))
	fprintf (outfd, "%s\n", horiLine);

    if (file_number == 0) {
	/* Aha, this is the toplevel file (title page) */
	fprintf (outfd, "<H1>\n%s</H1>\n", docTitle);

	if ((hh = hierList)) {
	    if (hierDepth == 0)
		hierIntList (hh, outfd);
	    else if (hierRefsOnTop)
		hierExtList (hh, outfd);

	    if (hierDepth == 0 || hierRefsOnTop) {
		if (hierContents) {
		    generateContents (0);
		    if (hierUseFrames)
			generateContents (1);
		    fprintf (outfd, 
			     "<UL>\n<LI><A HREF=\"%s%s.%s\"> %s </A>\n</UL>\n",
			     FPrefix, CSuffix, HtmlExt, titleContents);
		}

		fprintf (outfd, "%s\n", horiLine);
	    }
	    hierHasText = 0;
	} else
	    hierHasText = 1;
    } else {
	fprintf (outfd, "<H%d>\n%s</H%d>\n", 
		 hf->heading->level, hf->heading->text, hf->heading->level);

	if ((hh = hf->heading->fchild)) {
	    if (hierDepth < hh->level)
		hierIntList (hh, outfd);
	    else if (hierRefsOnTop)
		hierExtList (hh, outfd);

	    if (hierDepth < hh->level || hierRefsOnTop)
		fprintf (outfd, "%s\n", horiLine);

	    hierHasText = 0;
	} else
	    hierHasText = 1;
    }
} /* hierProlog */

/* ------------------------------------------------------------------------- */

static void hierExtList (hh, outfd)
HierHeading *hh;
FILE *outfd;
/* write list of external links to headings */
{
    fprintf (outfd, "<UL>\n");      
    while (hh) {
	fprintf (outfd, "<LI><A HREF=\"%s%s.%s\"> %s </A>\n", 
		 FPrefix, fsuffix (hh->filenum), HtmlExt, hh->text);
	hh = hh->peer;
    }
    fprintf (outfd, "</UL>\n");      
} /* hierExtList */

/* ------------------------------------------------------------------------- */

static HierHeading *hierIntList (hh, outfd)
HierHeading *hh;
FILE *outfd;
/* write list of internal links to headings (recursively!) */
{
    int level = hh->level; 

    fprintf (outfd, "<UL>\n");      
    while (hh) {
	fprintf (outfd, "<LI><A HREF=\"#Heading%d\">%s</A>\n", 
		 hh->number, hh->text);
     
	hh = hh->next;
	if (hh) {
	    if (hh->level > level)
		hh = hierIntList (hh, outfd);
	    if (hh)	/* bug fix 26.09.94 */
		if (hh->level < level)
		    break;
	}
    }
    fprintf (outfd, "</UL>\n");      
    return (hh);
} /* hierIntList */

/* ------------------------------------------------------------------------- */

void hierEpilog (outfd)
FILE *outfd;
/* Write an epilog to file outfd with number file_number. */
{
    HierHeading *hh;
    HierFile *hf;
    int i;

    if (hierHasText && (hierDepth > 0 || hierContents || hierIndex ))
	fprintf (outfd, "<P>%s\n", horiLine);

    if (!hierRefsOnTop) {

	if (file_number == 0) {
	    /* Aha, this is the toplevel file (title page) */
	    if ((hh = hierList)) {
		if (hierDepth > 0) {
		    hierExtList (hh, outfd);

		    if (hierContents) {
			generateContents (0);
			if (hierUseFrames)
			    generateContents (1);
			fprintf (outfd, 
				 "<UL>\n<LI><A HREF=\"%s%s.%s\"> %s </A>\n</UL>\n",
				 FPrefix, CSuffix, HtmlExt, titleContents);
		    }

		    fprintf (outfd, "%s\n", horiLine);
		}
	    }
	} else {
	    hf = hierFileList;
	    for (i = 1; i < file_number; i++)
		hf = hf->next;

	    if ((hh = hf->heading->fchild)) {
		if (hierDepth >= hh->level) {
		    hierExtList (hh, outfd);

		    fprintf (outfd, "%s\n", horiLine);
		}
	    }
	}

    } /* !hierRefsOnTop */

    hierNavPanel (outfd, 0);

} /* hierEpilog */

/* ------------------------------------------------------------------------- */

void hierChangeOutFile ()
/* RTFRead has found a heading, change output file if necessary */
{
    HierHeading *hh;
    int i;

    heading_number++;
    hh = hierList;
    for (i = 1; i < heading_number; i++)
	hh = hh->next;
    /* hh points to heading that has just been read */

#ifdef DEBUG
    fprintf (stderr, "L%d (%d, %d): %s\n", hh->level, inHeading,
	     groupInHeading, hh->text);
#endif
    if (hh->level <= hierDepth) {
	hierIsChangingFile = 1;
	HTMLCleanup ();
	HTMLInit ();
	hierIsChangingFile = 0;
    } else {
	FlushHTML ();
	UndoMarkup ();
	fprintf (hierFile, "\n%s<A NAME=\"Heading%d\">%s</A>%s\n", 
		 hh->start, hh->number, hh->text, hh->end);
    }
} /* hierChangeOutFile */

/* ========================================================================= */

static void generateContents (int framed)
/* creates a file for the table of contents */
{
    FILE *fd;
    char fname[256];
    int ofn;

    if (framed)
	sprintf (fname, "%s%s.%s", FPrefix, FCSuffix, HtmlExt);
    else
	sprintf (fname, "%s%s.%s", FPrefix, CSuffix, HtmlExt);

    fd = fopen(fname, "w");
    if (fd == NULL) {
	RTFPanic ("Open of %s Failed", fname);
    }

    ofn = file_number;
    file_number = -1;
#ifdef DEBUG
    fprintf (stderr, "Generating Table of Contents...\n");
#endif

    fprintf (fd, "<HTML><HEAD>\n");
    fprintf (fd, "%s", hierCreationString ());
    if (framed)
	fprintf (fd, "<BASE TARGET=%s2>\n", FPrefixR);
    fprintf (fd, "<TITLE> %s - %s </TITLE></HEAD>%s\n",
	     docTitle, titleContents, bodyTag ());

    if (hierNavPanel (fd, framed))
	fprintf (fd, "%s\n", horiLine);

    fprintf (fd, "<H1> %s - %s </H1>", docTitle, titleContents);
    hierContList (hierList, fd);

    fprintf (fd, "%s\n", horiLine);
    hierNavPanel (fd, framed);

    fprintf (fd, "</BODY></HTML>\n");
    fclose (fd);
    file_number = ofn;
} /* generateContents */

/* ------------------------------------------------------------------------- */

static HierHeading *hierContList (hh, outfd)
HierHeading *hh;
FILE *outfd;
/* recursively write list of external links to headings for the toc */
{
    int level = hh->level; 

    fprintf (outfd, "<UL>\n");      
    while (hh) {
	if (hh->level <= hierDepth)
	    fprintf (outfd, "<LI><A HREF=\"%s%s.%s\">%s</A>\n", 
		     FPrefix, fsuffix (hh->filenum), HtmlExt, hh->text);
	else
	    fprintf (outfd, "<LI><A HREF=\"%s%s.%s#Heading%d\">%s</A>\n", 
		     FPrefix, fsuffix (hh->filenum), HtmlExt,
		     hh->number, hh->text);

	hh = hh->next;
	if (hh) {
	    if (hh->level > level)
		hh = hierContList (hh, outfd);
	    if (hh)
		if (hh->level < level)
		    break;
	}
    }
    fprintf (outfd, "</UL>\n");      
    return (hh);
} /* hierContList */

/* ========================================================================= */

static void setNavTexts (lang)
int lang;
{
    static int oldLang = -1;

    if (!customNavPanel) {
	/* look in rtftohtml's library directory */
	hierSetNavPanel ("");
    }

    if (lang != oldLang) {

	/* this should be overhauled sometimes; it's getting quite long... */
	switch (lang) {
	case rtfLangFrench:
	    if (!customNavPanel) {
		strcpy (navText[NAV_PREVIOUS], FRE_PREVIOUS_TEXT);
		strcpy (navText[NAV_NEXT], FRE_NEXT_TEXT);
		strcpy (navText[NAV_UP], FRE_UP_TEXT);
		strcpy (navText[NAV_TITLE], FRE_TITLE_TEXT);
		strcpy (navText[NAV_CONTENTS], FRE_CONTENTS_TEXT);
		strcpy (navText[NAV_INDEX], FRE_INDEX_TEXT);
	    }
	    strcpy (titleContents, FRE_CONTENTS_TEXT);
	    strcpy (titleIndex, FRE_INDEX_TEXT);
	    oldLang = lang;
	    break;

	case rtfLangGerman:
	    if (!customNavPanel) {
		strcpy (navText[NAV_PREVIOUS], GER_PREVIOUS_TEXT);
		strcpy (navText[NAV_NEXT], GER_NEXT_TEXT);
		strcpy (navText[NAV_UP], GER_UP_TEXT);
		strcpy (navText[NAV_TITLE], GER_TITLE_TEXT);
		strcpy (navText[NAV_CONTENTS], GER_CONTENTS_TEXT);
		strcpy (navText[NAV_INDEX], GER_INDEX_TEXT);
	    }
	    strcpy (titleContents, GER_CONTENTS_TEXT);
	    strcpy (titleIndex, GER_INDEX_TEXT);
	    oldLang = lang;
	    break;

	case rtfLangItalian:
	    if (!customNavPanel) {
		strcpy (navText[NAV_PREVIOUS], ITA_PREVIOUS_TEXT);
		strcpy (navText[NAV_NEXT], ITA_NEXT_TEXT);
		strcpy (navText[NAV_UP], ITA_UP_TEXT);
		strcpy (navText[NAV_TITLE], ITA_TITLE_TEXT);
		strcpy (navText[NAV_CONTENTS], ITA_CONTENTS_TEXT);
		strcpy (navText[NAV_INDEX], ITA_INDEX_TEXT);
	    }
	    strcpy (titleContents, ITA_CONTENTS_TEXT);
	    strcpy (titleIndex, ITA_INDEX_TEXT);
	    oldLang = lang;
	    break;

	case rtfLangCastilianSpanish:
	    if (!customNavPanel) {
		strcpy (navText[NAV_PREVIOUS], CSP_PREVIOUS_TEXT);
		strcpy (navText[NAV_NEXT], CSP_NEXT_TEXT);
		strcpy (navText[NAV_UP], CSP_UP_TEXT);
		strcpy (navText[NAV_TITLE], CSP_TITLE_TEXT);
		strcpy (navText[NAV_CONTENTS], CSP_CONTENTS_TEXT);
		strcpy (navText[NAV_INDEX], CSP_INDEX_TEXT);
	    }
	    strcpy (titleContents, CSP_CONTENTS_TEXT);
	    strcpy (titleIndex, CSP_INDEX_TEXT);
	    oldLang = lang;
	    break;
	 
	default:
	    if (!customNavPanel) {
		strcpy (navText[NAV_PREVIOUS], PREVIOUS_TEXT);
		strcpy (navText[NAV_NEXT], NEXT_TEXT);
		strcpy (navText[NAV_UP], UP_TEXT);
		strcpy (navText[NAV_TITLE], TITLE_TEXT);
		strcpy (navText[NAV_CONTENTS], CONTENTS_TEXT);
		strcpy (navText[NAV_INDEX], INDEX_TEXT);
	    }
	    strcpy (titleContents, CONTENTS_TEXT);
	    strcpy (titleIndex, INDEX_TEXT);
	    oldLang = lang;
	}

    }
} /* setNavTexts */

/* ------------------------------------------------------------------------- */

void hierSetNavPanel (fname)
char *fname;
{
    FILE *fd;
    char buf[256], *cp;

    if (*fname)
	fd = fopen (fname, "r");
    else
	fd = RTFOpenLibFile (NAV_CONFIG_FILE, "r");
    
    if (!fd) {
	if (*fname)
	    perror (fname);
	return;
    }

    strcpy (navText[NAV_PREVIOUS], PREVIOUS_TEXT);
    strcpy (navText[NAV_NEXT], NEXT_TEXT);
    strcpy (navText[NAV_UP], UP_TEXT);
    strcpy (navText[NAV_TITLE], TITLE_TEXT);
    strcpy (navText[NAV_CONTENTS], CONTENTS_TEXT);
    strcpy (navText[NAV_INDEX], INDEX_TEXT);
    strcpy (navText[NAV_DELIM], ", ");
    strcpy (navText[NAV_NOPREVIOUS], "");
    strcpy (navText[NAV_NONEXT], "");
    strcpy (navText[NAV_NOUP], "");
    strcpy (navText[NAV_NOTITLE], "");
    
    while (!feof (fd)) {
	fgets (buf, 254, fd);
	if (*buf && *buf != '#') {
	    cp = strchr (buf, ':');
	    if (cp) {
		*cp++ = 0;
		/* cut trailing newline */
		*(cp + strlen (cp) - 1) = 0;
		
		if (!strncmp (buf, "delim", 5)) {
		    strcpy (navText[NAV_DELIM], cp);
		} else if (!strncmp (buf, "prev", 4)) {
		    strcpy (navText[NAV_PREVIOUS], cp);
		} else if (!strncmp (buf, "next", 4)) {
		    strcpy (navText[NAV_NEXT], cp);
		} else if (!strncmp (buf, "up", 2)) {
		    strcpy (navText[NAV_UP], cp);
		} else if (!strncmp (buf, "title", 5)) {
		    strcpy (navText[NAV_TITLE], cp);
		} else if (!strncmp (buf, "cont", 4)) {
		    strcpy (navText[NAV_CONTENTS], cp);
		} else if (!strncmp (buf, "index", 5)) {
		    strcpy (navText[NAV_INDEX], cp);
		} else if (!strncmp (buf, "noprev", 6)) {
		    strcpy (navText[NAV_NOPREVIOUS], cp);
		} else if (!strncmp (buf, "nonext", 6)) {
		    strcpy (navText[NAV_NONEXT], cp);
		} else if (!strncmp (buf, "noup", 4)) {
		    strcpy (navText[NAV_NOUP], cp);
		} else if (!strncmp (buf, "notitle", 7)) {
		    strcpy (navText[NAV_NOTITLE], cp);
		} else if (!strncmp (buf, "hr", 2)) {
		    strcpy (horiLine, cp);
		} else if (!strncmp (buf, "bgimage", 7)) {
		    strcpy (bodyBgImage, cp);
		} else if (!strncmp (buf, "bgcolor", 7)) {
		    strcpy (bodyBgColor, cp);
		} else if (!strncmp (buf, "textcol", 7)) {
		    strcpy (bodyTextColor, cp);
		} else if (!strncmp (buf, "link", 4)) {
		    strcpy (bodyLinkColor, cp);
		} else if (!strncmp (buf, "vlink", 5)) {
		    strcpy (bodyVLinkColor, cp);
		} else if (!strncmp (buf, "alink", 5)) {
		    strcpy (bodyALinkColor, cp);
		}
	    }
	}
    }

    fclose (fd);
    
    customNavPanel = 1;
}

/* ------------------------------------------------------------------------- */

static int hierNavPanel (outfd, framed)
FILE *outfd;
int framed;
{
    HierFile *hf;
    HierHeading *hh;
    int i;
    long pos;
    char delim[32];

    pos = ftell (outfd);

    if (customNavPanel)
	strcpy (delim, navText[NAV_DELIM]);
    else
	strcpy (delim, ", ");

    if (file_number == -2) {
	/* Index */
	if (num_files > 1) {
	    if (!framed)
		fprintf (outfd, "<A HREF=\"%s%s.%s\">%s</A>\n", 
			 FPrefix, (hierUseFrames ? TSuffix : ""),
			 HtmlExt, navText[NAV_TITLE]);
	    else
		fprintf (outfd, "<A HREF=\"%s%s.%s\" TARGET=%s2>%s</A>\n", 
			 FPrefix, TSuffix, HtmlExt, FPrefixR,
			 navText[NAV_TITLE]);
	    if (hierContents)
		fprintf (outfd, "%s", delim);
	}
    } else if (file_number == -1) {
	/* Table of Contents */
	if (num_files > 1) {
	    if (!framed)
		fprintf (outfd, "<A HREF=\"%s%s.%s\">%s</A>\n", 
			 FPrefix, (hierUseFrames ? TSuffix : ""),
			 HtmlExt, navText[NAV_TITLE]);
	    else
		fprintf (outfd, "<A HREF=\"%s%s.%s\" TARGET=%s2>%s</A>\n", 
			 FPrefix, TSuffix, HtmlExt, FPrefixR,
			 navText[NAV_TITLE]);
	}
    } else if (file_number == 0) {
	/* Title Page */
	if (num_files > 1) {
	    if (*navText[NAV_NOPREVIOUS])
		fprintf (outfd, "%s%s\n", navText[NAV_NOPREVIOUS], delim);
		
	    fprintf (outfd, "<A HREF=\"%s%s.%s\">%s</A>\n", 
		     FPrefix, fsuffix (1), HtmlExt, navText[NAV_NEXT]);
	    
	    if (*navText[NAV_NOUP])
		fprintf (outfd, "%s%s\n", delim, navText[NAV_NOUP]);
	    if (*navText[NAV_NOTITLE])
		fprintf (outfd, "%s%s\n", delim, navText[NAV_NOTITLE]);
	    
	    if (hierContents || hierIndex)
		fprintf (outfd, "%s", delim);
	}
    } else {
	hf = hierFileList;
	for (i = 1; i < file_number; i++)
	    hf = hf->next;

	hh = hf->heading;
	if (hh->filenum == 1)
	    fprintf (outfd, "<A HREF=\"%s%s.%s\">%s</A>\n", 
		     FPrefix, fsuffix (0), HtmlExt, navText[NAV_PREVIOUS]);
	else
	    fprintf (outfd, "<A HREF=\"%s%s.%s\">%s</A>\n", 
		     FPrefix, fsuffix (hh->filenum - 1), HtmlExt,
		     navText[NAV_PREVIOUS]);
	if (hh->filenum < num_files - 1) {
	    fprintf (outfd, "%s<A HREF=\"%s%s.%s\">%s</A>\n", delim,
		     FPrefix, fsuffix (hh->filenum + 1), HtmlExt,
		     navText[NAV_NEXT]);
	} else if (*navText[NAV_NONEXT])
	    fprintf (outfd, "%s%s\n", delim, navText[NAV_NONEXT]);
	
	if (hierDepth > 1) {
	    if (hh->father)
		fprintf (outfd, "%s<A HREF=\"%s%s.%s\">%s</A>\n", delim,
			 FPrefix, fsuffix (hh->father->filenum), HtmlExt,
			 navText[NAV_UP]);
	    else {
		fprintf (outfd, "%s<A HREF=\"%s%s.%s\">%s</A>\n", delim,
			 FPrefix, fsuffix (0), HtmlExt, navText[NAV_UP]);
	    }
	}
	fprintf (outfd, "%s<A HREF=\"%s%s.%s\">%s</A>\n", delim,
		 FPrefix, fsuffix (0), HtmlExt, navText[NAV_TITLE]);
	if (hierContents || hierIndex)
	    fprintf (outfd, "%s", delim);
    }

    if (hierContents && file_number != -1) {
	if (file_number == -2 && framed) 
	    fprintf (outfd, "<A HREF=\"%s%s.%s\" TARGET=%s1>%s</A>\n", 
		     FPrefix, FCSuffix, HtmlExt, FPrefixR,
		     navText[NAV_CONTENTS]);
	else
	    fprintf (outfd, "<A HREF=\"%s%s.%s\">%s</A>\n", 
		     FPrefix, CSuffix, HtmlExt, navText[NAV_CONTENTS]);
    }

    if (hierIndex && file_number != -2) {
	if (file_number == -1 && framed)
	    fprintf (outfd, "%s<A HREF=\"%s%s.%s\" TARGET=%s1>%s</A>\n",
		     (ftell(outfd) != pos ? delim : ""),
		     FPrefix, FISuffix, HtmlExt, FPrefixR,
		     navText[NAV_INDEX]);
	else
	    fprintf (outfd, "%s<A HREF=\"%s%s.%s\">%s</A>\n", 
		     (ftell(outfd) != pos ? delim : ""),
		     FPrefix, ISuffix, HtmlExt, navText[NAV_INDEX]);
    }


    if (ftell (outfd) != pos)
	return (1);
    else
	return (0);

} /* hierNavPanel */

/* ========================================================================= */

int colorIsRed (colnum)
int colnum;
{
    RTFColor *colptr;

    if ((colptr = RTFGetColor (colnum))) {
	if (colptr->rtfCRed == 255 && !colptr->rtfCGreen && !colptr->rtfCGreen)
	    return(1);
	else
	    return(0);
    } else
	return(0);
} /* colorIsRed */

/* ------------------------------------------------------------------------- */

void hierResolve ()
{
    HierHeading *hh = hierList, *refhh = NULL;
    char buf[512];
    int i;

#ifdef DEBUG2
    fprintf (stderr, "Resolving: %s\n", headingRef);
#endif

    while (hh) {
	if (!alpha_strncmp (hh->text, headingRef, strlen (headingRef))) {
	    refhh = hh;
	    break;
	}

	hh = hh->next;
    }
    if (!refhh) {
	/* this is for email and URI references, it skips blank chars */
	for (i = 0; headingRef[i]; i++) {
	    if (headingRef[i] != ' ' && headingRef[i] != '\n' &&
		headingRef[i] != '\t')
		buf[i] = headingRef[i];
	}
	buf[i] = '\0';
    }

    FlushHTML ();

    if (refhh) {
	if (refhh->level <= hierDepth)
	    fprintf (hierFile, "<A HREF=\"%s%s.%s\">%s</A>", 
		     FPrefix, fsuffix (refhh->filenum), HtmlExt, headingRef);
	else {
	    if (refhh->filenum != file_number)
		fprintf (hierFile, "<A HREF=\"%s%s.%s#Heading%d\">%s</A>", 
			 FPrefix, fsuffix (refhh->filenum), HtmlExt,
			 refhh->number, headingRef);
	    else
		fprintf (hierFile, "<A HREF=\"#Heading%d\">%s</A>", 
			 refhh->number, headingRef);
	}
    } else if (strchr (headingRef, '@')) {
	/* assume headingRef to contain an email address */
	fprintf (hierFile, "<A HREF=\"mailto:%s\">%s</A>", buf, headingRef);
    } else if (strchr (headingRef, ':')) {
	/* assume headingRef to contain a URI */
	fprintf (hierFile, "<A HREF=\"%s\">%s</A>", buf, headingRef);
    } else {
	fprintf (stderr, "Warning: Couldn't resolve \"%s\"!\n", headingRef);
	fprintf (hierFile, "%s", headingRef);
    }
} /* hierResolve */

/* ========================================================================= */

HierIndex *addIndexEntry ()
/* adds indexEntry to list of index entries and inserts an anchor */
/* Note: This does not happen in the pre-pass, but in the main-pass! */
/* Well, for indices in headings it actually _does_ happen int the pre-pass. */
{
    HierIndex *hi;

#ifdef DEBUG2
    fprintf (stderr, "indexEntry: %s\n", indexEntry);
#endif

    hi = newIndexEntry ();
    hi->number = ++num_index_entries;
    hi->filenum = file_number;
    strcpy (hi->text, indexEntry);
    strcpy (indexEntry, "");

    /* now insert an anchor into the current html-file */
    if (hierFile) {
	FlushHTML ();
	fprintf (hierFile, "<A NAME=\"Index%d\">%s</A>", 
		 num_index_entries, indexAnchorText);
    }

    return (hi);
} /* addIndexEntry */

/* ------------------------------------------------------------------------- */

static HierIndex *newIndexEntry ()
{
    HierIndex *newhi, *hi;

    newhi = (HierIndex *) RTFAlloc (sizeof (HierIndex));
#ifdef DEBUGM
    fprintf (stderr, "RTFAlloc in newIndexEntry returns: %p\n", newhi);
#endif
    if (!newhi)
	RTFPanic("Not enough memory in newIndexEntry!\n");

    newhi->next = NULL;
    newhi->heading = NULL;
    newhi->number = newhi->filenum = 0;
    newhi->level = 1;
    newhi->textp = newhi->text;
    strcpy(newhi->text, "");

    hi = hierIndexList;
    if (hi == NULL)
	hierIndexList = newhi;
    else {
	while (hi->next)
	    hi = hi->next;
	hi->next = newhi;
    }

    return (newhi);
} /* newIndexEntry */

/* ------------------------------------------------------------------------- */

void generateIndex (int framed)
/* creates a file for the index */
{
    FILE *fd;
    char fname[256], fs[16], ofs[16], ohit[HEADINGSIZE+2], *hitp, *lip;
    static char odel[] = " ", oli[] = "", li[] = "<LI>";
    int i = 0, j, ii = 1, ofn, olevel = 1;
    HierIndex *hi;
    HierHeading *hh;

    if (!hierIndexList) {
	fprintf (stderr, "Warning: no index entries were found\n");
	return;
    }

    if (framed)
	sprintf (fname, "%s%s.%s", FPrefix, FISuffix, HtmlExt);
    else
	sprintf (fname, "%s%s.%s", FPrefix, ISuffix, HtmlExt);

    fd = fopen(fname, "w");
    if (fd == NULL) {
	RTFPanic ("Open of %s Failed", fname);
    }

#ifdef DEBUG
    fprintf (stderr, "Generating Index...\n");
#endif

    fprintf (fd, "<HTML><HEAD>\n");
    fprintf (fd, "%s", hierCreationString ());
    if (framed)
	fprintf (fd, "<BASE TARGET=%s2>\n", FPrefixR);
    fprintf (fd, "<TITLE> %s - %s </TITLE></HEAD>%s\n",
	     docTitle, titleIndex, bodyTag ());

    ofn = file_number;
    file_number = -2;
    if (hierNavPanel (fd, framed))
	fprintf (fd, "%s\n", horiLine);
    fprintf (fd, "<H1> %s - %s </H1>\n", docTitle, titleIndex);

    hi = sortIndex ();
    hi = preprocessIndex ();

    /* write index-panel */
    fs[0] = ofs[0] = '\0';
    fprintf (fd, "<H2>\n");
    while (hi) {
	fs[0] = hi->text[0];
	if (fs[0] != '&') {
	    fs[1] = '\0';
	    if (islower (fs[0]))
		fs[0] -= 0x20;
	} else {
	    j = 0;
	    /* this looks wrong but it works */
	    while (hi->text[j++] != ';')
		fs[j] = hi->text[j];
	    fs[j] = '\0';
	}
	if (strcmp (fs, ofs)) {
	    if (ofs[0])
		fprintf (fd, " , ");
	    strcpy (ofs, fs);
	    if (!framed)
		fprintf (fd, "<A HREF=\"#IA-%s\">%s</A>\n", fs, fs);
	    else
		fprintf (fd, "<A HREF=\"#IA-%s\" TARGET=%s1>%s</A>\n",
			 fs, FPrefixR, fs);
	}

	hi = hi->next;
    };
    fprintf (fd, "</H2>%s\n", horiLine);

    /* now write index */
    fs[0] = ofs[0] = ohit[0] = '\0';
    hi = hierIndexList;
    while (hi) {
	if (hi->level < olevel) {
	    for (; olevel > hi->level; olevel--)
		fprintf (fd, "</UL>\n");
	}
	fs[0] = hi->text[0];
	if (fs[0] != '&') {
	    fs[1] = '\0';
	    if (islower (fs[0]))
		fs[0] -= 0x20;
	} else {
	    j = 0;
	    /* this looks wrong but it works */
	    while (hi->text[j++] != ';')
		fs[j] = hi->text[j];
	    fs[j] = '\0';
	}
	if (strcmp (fs, ofs)) {
	    strcpy (ofs, fs);
	    if (i > 0) {
		fprintf (fd, "\n</UL>");
	    }
	    fprintf (fd, "\n<H2><A NAME=\"IA-%s\">%s</A></H2><UL>\n",
		     fs, fs);
	}

	if (*hi->textp) {
	    strcpy (ohit, hi->textp);
	    hitp = trimleft (ohit, " ,:");
	    lip = li;
	    ii = 1;
	} else {
	    hitp = odel;
	    lip = oli;
	    ii++;
	}

	if (hi->level > olevel) {
	    olevel = hi->level;
	    fprintf (fd, "<UL>\n");
	}
	
	if (!hi->heading) {
	    if (hi->number >= 0)
		fprintf (fd, "%s%s <A HREF=\"%s%s.%s#Index%d\">[%d]</A>\n", 
			 lip, hitp, FPrefix, fsuffix (hi->filenum), HtmlExt,
			 hi->number, ii);
	    else
		/* it's an index "headline" */
		fprintf (fd, "%s%s\n", lip, hitp);
	} else {
	    hh = hi->heading;
	    if (hh->level <= hierDepth)
		fprintf (fd, "%s%s <A HREF=\"%s%s.%s\">[%d]</A>\n", 
			 lip, hitp, FPrefix, fsuffix (hh->filenum), HtmlExt,
			 ii);
	    else
		fprintf (fd, "%s%s <A HREF=\"%s%s.%s#Heading%d\">[%d]</A>\n", 
			 lip, hitp, FPrefix, fsuffix (hh->filenum), HtmlExt,
			 hh->number, ii);
	}
	hi = hi->next;
	if (!hi) {
	    for (; olevel > 1; olevel--)
		fprintf (fd, "</UL>\n");
	    break;
	}
	i++;
    }

    fprintf (fd, "</UL>%s\n", horiLine);   
    hierNavPanel (fd, framed);
    file_number = ofn;

    fprintf (fd, "</BODY></HTML>\n");   
    fclose (fd);

} /*generateIndex */


void hierSetIndexAnchor (text)
char *text;
{
    strcpy (indexAnchorText, text);
} /* hierSetDocTitle */

/* ------------------------------------------------------------------------- */

static HierIndex *sortIndex ()
/* There certainly are smarter sorting algorithms, but I really don't care
   too much about that at the moment... */
{
    HierIndex *sortedList, *hi, *hj, *hp;

    sortedList = NULL;

    while (hierIndexList) {
	hi = hierIndexList;
	hierIndexList = hi->next;
	hi->next = NULL;

	if (!sortedList)
	    sortedList = hi;
	else {
	    hj = sortedList;
	    hp = NULL;
	    while (hj) {
		if (alpha_strcmp (hi->text, hj->text) < 0) {
		    if (!hp)
			sortedList = hi;
		    else
			hp->next = hi;
		    hi->next = hj;
		    break;
		}
		hp = hj;
		hj = hj->next;
	    };
	    if (!hj)
		hp->next = hi;
	}
    };

    return ((hierIndexList = sortedList));
} /* sortIndex */

/* ------------------------------------------------------------------------- */

static HierIndex *preprocessIndex ()
/* Collapses the index entries, resulting in a multi-level, clearer index. */
{
    HierIndex *hi = hierIndexList, *newhi, *prehi = NULL;
    int len, len2, m, oldm = 0;

    if (!hierIndexList) return NULL;
    
    while (hi->next) {
	len = strlen (hi->text);
	len2 = strlen (hi->next->text);
	m = matchOffset (hi->text, hi->next->text);
	if (m < oldm || m == 0) {
	    m = 0;
	    hi->next->level = 1;
	} else if (m > oldm && oldm == 0) {
	new_index_group:
	    /* hi->level == 1 is guaranteed */
	    if (m < len) {
		/* create new index "heading" by cutting the current one */
		newhi = (HierIndex *) RTFAlloc (sizeof (HierIndex));
		if (!newhi)
		    RTFPanic("Not enough memory in preprocessIndex!\n");

		if (prehi)
		    prehi->next = newhi;
		else
		    hierIndexList = prehi;
		newhi->next = hi;
		newhi->heading = NULL;
		newhi->number = -1;
		newhi->level = 1;
		newhi->textp = newhi->text;
		strncpy(newhi->text, hi->text, m);
		newhi->text[m] = '\0';
		hi->level = 2;
		hi->textp = hi->text + m;
	    }
	    if (m < len2) {
		hi->next->level = 2;
	    } else
		m = len2;
	    hi->next->textp = hi->next->text + m;
	} else if (m == oldm) {
	    hi->next->level = 2;
	    hi->next->textp = hi->next->text + m;
	} else /* if (m > oldm && oldm > 0) */ {
	    if (m == len && m == len2) {
		hi->next->textp = hi->next->text + m;
		hi->next->level = 2;
		m = oldm;
	    } else {
		if (*hi->textp) {
		    hi->level = 1;
		    hi->textp = hi->text;
		    goto new_index_group; /* ouch, that hurts! */
		} else {
		    /* current heading is a perfect match of the preceding
		       one --> prevent splitting of these */
		    m = oldm;
		    hi->next->textp = hi->next->text + m;
		    hi->next->level = 2;
		}
	    }
	}
	oldm = m;
	prehi = hi;
	hi = hi->next;
    }

    return (hierIndexList);
} /* preprocessIndex */

static int matchOffset (s1, s2)
char *s1, *s2;
/* If n leading words of s1 and s2 match, returns the offset of the first
   non-matching char */
{
    char *s10 = s1, *ld1 = NULL;

    for (; *s1 == *s2 && *s1 && *s2; s1++, s2++) {
	if (ISIDEL(*s1)) {
	    ld1 = s1;
	}
    }
    
    if ((!*s1 && !*s2) || /* perfect match */
	(!*s1 && ISIDEL(*s2)) || /* e.g. "abc" and "abc, def" */
	(!*s2 && ISIDEL(*s1)) ||
	(ISIDEL(*s1) && ISIDEL (*s2))) /* e.g. "abc, def" and "abc: def" */
	return ((int) (s1 - s10));
    else if (!ISIDEL(*s1) || !ISIDEL(*s2)) {
	if (!ld1) /* no match */
	    return 0;
	else /* e.g. "abc def" and "abc dex" */
	    return ((int) (ld1 - s10));
    } else
	return 0;
} /* matchOffset */

static char *trimleft (str, trim)
char *str, *trim;
/* Cuts all chars in trim from the start of str. */
{
    while (strchr (trim, *str) && *str) str++;
    return str;
}

/* ------------------------------------------------------------------------- */

static void generateFrames ()
/* generates the frames title file if -F was given */
{
    char fname[256], buf[BUFSIZ];
    int in_body = 0;
    FILE *fd, *tfd;

    sprintf (fname, "%s.%s", FPrefix, HtmlExt);

    fd = fopen(fname, "w");
    if (fd == NULL) {
	RTFPanic ("Open of %s Failed", fname);
    }

#ifdef DEBUG
    fprintf (stderr, "Generating Frames...\n");
#endif

    fprintf (fd, "<HTML><HEAD>\n");
    fprintf (fd, "%s", hierCreationString ());
    fprintf (fd, "<TITLE>%s</TITLE></HEAD>\n\n", docTitle);
    fprintf (fd, "<FRAMESET COLS=\"40%%,60%%\">\n\n<NOFRAMES>\n");

    /* now read contents of Title file */
    sprintf (fname, "%s%s.%s", FPrefix, TSuffix, HtmlExt);
    tfd = fopen(fname, "r");
    if (tfd == NULL) {
	RTFPanic ("Open of %s Failed", fname);
    }
    while (1) {
	if (!fgets (buf, BUFSIZ, tfd))
	    break;
	if (!alpha_strncmp (buf, "<body>", 6))
	    in_body = 1;
	if (!alpha_strncmp (buf, "</body>", 7)) {
	    fputs ("</body>\n", fd);
	    break;
	}
	if (in_body)
	    fputs (buf, fd);
    }
    fclose (tfd);
    
    fprintf (fd, "</NOFRAMES>\n\n");
    fprintf (fd, "<FRAME SRC=%s%s.%s NAME=%s1>\n", FPrefix, FCSuffix, HtmlExt,
	     FPrefixR);
    fprintf (fd, "<FRAME SRC=%s%s.%s NAME=%s2>\n", FPrefix, TSuffix, HtmlExt,
	     FPrefixR);
    fprintf (fd, "</FRAMESET>\n\n</HTML>\n");

    fclose (fd);
} /* generateFrames */

/* ------------------------------------------------------------------------- */

static int alpha_strcmp (s1, s2)
char *s1, *s2;
/* comparing function for sortIndex (emulates strcasecmp) */
{
    char c1, c2;

    while (*s1 && *s2) {
	if (*s1 != *s2) {
	    c1 = *s1;
	    c2 = *s2;

	    if (islower (c1))
		c1 -= 0x20;
	    if (islower (c2))
		c2 -= 0x20;

	    if (c1 < c2)
		return (-1);
	    if (c1 > c2)
		return (1);
	}

	s1++;
	s2++;
    };

    if (!*s1 && !*s2)
	return (0);
    if (!*s1)
	return (-1);
    else
	return (1);
} /* alpha_strcmp */

static int alpha_strncmp (s1, s2, n)
char *s1, *s2;
int n;
/* emulates strncasecmp */
{
    char c1, c2;
    int i = 0;

    while (*s1 && *s2 && i++ < n) {
	if (*s1 != *s2) {
	    c1 = *s1;
	    c2 = *s2;

	    if (islower (c1))
		c1 -= 0x20;
	    if (islower (c2))
		c2 -= 0x20;

	    if (c1 < c2)
		return (-1);
	    if (c1 > c2)
		return (1);
	}

	s1++;
	s2++;
    };

    if ((!*s1 && !*s2) || i == n)
	return (0);
    if (!*s1)
	return (-1);
    else
	return (1);
} /* alpha_strncmp */

/* ------------------------------------------------------------------------- */

static char *str_replace (str, this, that)
char *str, *this, *that;
/* Replaces all occurences of this in str by that. */
{
    static char res[512];
    char *resp = res;
    int thislen = strlen (this), thatlen = strlen (that);

    while (*str) {
	if (*str != this[0])
	    *resp++ = *str++;
	else {
	    if (!strncmp (str, this, thislen)) {
		strncpy (resp, that, thatlen);
		str += thislen;
		resp += thatlen;
	    } else
		*resp++ = *str++;
	}
    }
    *resp = 0;
    
    return res;
} /* str_replace */

/* ------------------------------------------------------------------------- */

void hierSetDocTitle (title)
char *title;
/* Set the document title. Receives the argument of the -T cmdline option. */
{
    strcpy (docTitle, title);
} /* hierSetDocTitle */

/* ------------------------------------------------------------------------- */

char *hierCutShort (file)
char *file;
/* Cuts the file resp. its basename to a maximum of 6 chars. */
{
    int len = strlen (file);
    char *ls = strrchr (file, '/'), *fp = file;

    if (ls) {
	fp = ls + 1;
	len -= (int) ls - (int) file + 1;
    }

    if (len > 6)
	*(fp + 6) = 0;

    return (file);
} /* hierCutShort */

#endif /* CB_ADD */





