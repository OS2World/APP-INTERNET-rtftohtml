#include	<stdio.h>
#include	"rtf.h"
#include	"rtftohtml.h"
#include	"string.h"

#ifdef CB_ADD
#include	"hier.h"
#endif

static void 	AppendText();


static void	PutHex ();
#ifndef CB_ADD
static char	*MapChar();
#endif /* ifdef CB_ADD, MapChar is declared in rtftohtml.h */

#define MAXNEST 8

/* Destinations can be files or strings.  */


struct SDest

HName = {
    NULL, 0, 0
},
HHref = {
    NULL, 0, 0
},
Title = {
    NULL, 0, 0
},
SkipCol = {
    NULL, 0, 0
},
BkName = {
    NULL, 0, 0
},
Item = {
    NULL, 0, 0
};

/* SSC codes are used in saving the state */
#define SSCSPECIAL	0
#define SSCSTYLECHG	1
#define SSCNCODES	2	/* Set this to number of SSCodes */

struct SSData {
	int Special;
	int cpos;
	InStateStack *TheState;
};

struct SSStruct {
	struct SDest SD;
	int alloc;
	int used;
	struct SSData *data;
} PNSave,FNSave;

TMatchRec FNMatch;

char * (badstyles[256]);	
int anyfoot;
int anyTOC;
int InBody = 0;
int PNText = 0;
int FNText = 0;
int HToCLev = 0;
int oToCLev = 0;
int ToCidx=0;
int ToCText=0;

struct FDest {
    char filename[256];		/* Name of output file */
    char wbuff[82];		/* Output buffer */
    FILE *outfd;		/* Output file - (file destinations only) */
    int wrap;		    /* should we wrap text or not */
    int HPTags;			/* Highest Paragraph Tag */
    int HTTags;			/* Highest Text Tag */
    int NeedPar;		/* Output Paragraph before next text */
    int InCol2;			/* We are past Column 1 */
    int PTidx;			/* Paragraph Tag Index */
    int TSpecial;		/* Special Tag */
    int cpos;			/* character position */
    int TStack[MAXNEST];	/* Stack of tags */
}
 EmptyFD = {
    "", "",NULL,1, -1, -1, 0, 0, 0,0, 0
},
 File1 = {
    "","", NULL,1, -1, -1, 0, 0, 0,0, 0
},
ToCFile = {
    "","", NULL,1, -1, -1, 0, 0, 0,0, 0
},
Footnote = {
    "","", NULL,1, -1, -1, 0, 0, 0,0, 0
};

#define ConvertCaps	((TStyle&(styleAllCaps|styleSmallCaps))!=0)
int outfuniq = 0;

char *LineBuffer = (char *) NULL;

FILE *PictFile;
int uniqnum = 0;

#define MAXOBUF	8096
int
nextfield(cp, w,  ob,  adj,  EntIsOne)
char *cp;int w; char *ob; int adj; int EntIsOne;
{
	int i,j,br = -1,lp,rp,nb= -1,ow=w;
	for(i=0;i<w&&cp[i]!='\0';i++){
		if(cp[i]==' '||cp[i]=='\t'){
			if(nb==i-1)br=i;
		}	else {
			if(EntIsOne){
				if(cp[i]=='&'){
					while(cp[i]!=';'&&cp[i]!='\0'){ 
						w++;
						i++;
					}
				} else if(cp[i]=='<'){
				    while(cp[i]!='>'&&cp[i]!='\0'){
						w++;
						i++;
					}
				}
			}
			nb=i;
		}
	}
	if(i==0){
		ob[0]='\0';
		return(0);
	}
	if(cp[i]=='\0'&&nb==i-1)br=i;
	if(br==-1)br=w;
	if(w!=ow){ /* now subtract off any entities not used */
		for(i=br,j=0;i<w&&cp[i]!='\0';i++){
			if(cp[i]=='&'){
				while(cp[i]!=';'){
					j++;
					i++;
				}
			}
		}
		w-=j;
	}
	switch(adj){
	case AdjL:			/* left justify - no pad */
		lp=0;
		rp=br;
		break;
	case AdjLPad:			/* left justify */
		lp=0;
		rp=w;
		break;
	case AdjRPad:			/* right justify */
		lp=w-br;
		rp=w;
		break;
	case AdjCpad:			/* center */
		lp=(w-br)/2;
		rp=w;
		break;
	}
	if(rp>=MAXOBUF){
		RTFPanic ("Overflowed MAXOBUF too many Entities in a table cell");
	}
	for(i=0;i<lp;i++)ob[i]=' ';
	if (br == w && !EntIsOne)
	  /* don't lose the last character from cp! */
	  for(j=0;j<br+1;i++,j++)ob[i]=cp[j];
	else
	for(j=0;j<br;i++,j++)ob[i]=cp[j];
	for(;i<rp;i++)ob[i]=' ';
	ob[i]='\0';
	return(br);
}

void
putwrap( s,  OD, wrap)
char *s;struct FDest *OD;int wrap;
{
	int i;
	int n;
	char obuff[133];
	/* If writing to a table, just save up the text */
	if(!wrap){
#ifdef CB_ADD
	    if (OD->TSpecial == MTHref && strlen(s) == 1) {
		strcat (anchorBuf, s);
		return;
	    }
#endif
		if(OD->cpos>0){
			OD->wbuff[OD->cpos]='\0';
			fputs(OD->wbuff,OD->outfd);
			fputc('\n',OD->outfd);
		}
		fputs(s,OD->outfd);
		OD->cpos= -1;
		return;
	}
	i=OD->cpos;
	while( *s != '\0'){
		if(i==81){
			n=nextfield(OD->wbuff, 80,  obuff, AdjL,0);
			fputs(obuff,OD->outfd);
			if(n<80){	/* only if we have a break */
				fputc('\n',OD->outfd);
			    for(;n<81;n++)
					if(OD->wbuff[n]!=' '&&OD->wbuff[n]!='\t')
						break;
			    for(i=0;n<81;i++,n++)
			    	OD->wbuff[i]=OD->wbuff[n];
			} else  { /* force break at next opportunity */
				i= -1;
			}
		}
		if(i<0){
			if(strchr(" \t\n",*s)!=NULL){ /*found break */
				fputc('\n',OD->outfd);
				i=0;
				s++;
			} else{
				fputc(*(s++),OD->outfd);
			}
		} else if(*s=='\n'){
			OD->wbuff[i]='\0';
			fputs(OD->wbuff,OD->outfd);
			fputc(*(s++),OD->outfd);
			i=0;
			continue;
		} else
			OD->wbuff[i++] = *(s++);
	}
	OD->cpos=i;
}

HTMLInit()
{
	int i;
#ifdef CB_ADD
static int fnFileInit = 0;
#endif

	if (LineBuffer == (char *) NULL
			&& (LineBuffer = RTFAlloc (512)) == (char *) NULL)
		RTFPanic ("cannot allocate line buffer.");
#ifdef CB_ADD
	if (hierPass < 2) {
	    for(i=0;i<256;i++)
		badstyles[i]=NULL;
	}
#else
	for(i=0;i<256;i++)
		badstyles[i]=NULL;
#endif
	HName.used=0;
	HHref.used=0;
	Title.used=0;
	Item.used=0;
	SkipCol.used=0;
	File1=EmptyFD;
#ifndef CB_ADD
	Footnote=EmptyFD;
#else
	if (!fnFileInit)
	    Footnote=EmptyFD;	/* this shouldn't be necessary anyway */
#endif
	ToCFile=EmptyFD;
#ifndef CB_ADD
	outfuniq=0;
	uniqnum=0;
#else
	if (!fnFileInit) {
	  outfuniq=0;
	  uniqnum = 0;
	}
#endif
	ToCidx=1;
	ToCText=0;
#ifndef CB_ADD
	anyfoot=0;
#else
	if (!hierPass || !hierIsChangingFile)
	anyfoot=0;
#endif
	anyTOC=0;
	InBody = 0;
	HToCLev=0;
	oToCLev = 0;
	for(i=0;i<PNSave.used;i++){
		if(PNSave.data[i].TheState){
			    RTFFree(PNSave.data[i].TheState);
		}
	}
	PNSave.SD.used=0;
	PNSave.used=0;
	for(i=0;i<FNSave.used;i++){
		if(FNSave.data[i].TheState){
			    RTFFree(FNSave.data[i].TheState);
		}
	}
	FNSave.SD.used=0;
	FNSave.used=0;
	FNMatch.Font= "NotAFont";

	
#ifndef CB_ADD	
    strcpy(File1.filename, OutfileName);
#else
    if (hierPass)
       strcpy(File1.filename, hierOutFile());
    else
    strcpy(File1.filename, OutfileName);
#endif
    if (OutfileName[0] != '\0') {
	File1.outfd = OpenOutputFile(File1.filename, "w",FTTEXT);
	if (File1.outfd == NULL) {
	    RTFPanic ("Open of %s Failed", File1.filename);
	}
    } else {
	File1.outfd = stdout;
    }
    /* open a file for footnotes */
#ifndef CB_ADD
    sprintf(Footnote.filename, "%s_fn.html", FPrefix);
	Footnote.outfd = OpenOutputFile(Footnote.filename, "w",FTTEXT);
	if (Footnote.outfd == NULL) {
	    RTFPanic ("Open of %s Failed", Footnote.filename);
	}
#else
    if (!fnFileInit) {
	if (!hierShortFileNames)
	    sprintf(Footnote.filename, "%s_fn.html", FPrefix);
	else
	    sprintf(Footnote.filename, "%sfn.htm", FPrefix);
	Footnote.outfd = OpenOutputFile(Footnote.filename, "w",FTTEXT);
	if (Footnote.outfd == NULL) {
	    RTFPanic ("Open of %s Failed", Footnote.filename);
	}
    }
#endif
	if(ToC){
	    /* open a file for Table of Contents */
	    sprintf(ToCFile.filename, "%s_ToC.html", FPrefix);
		ToCFile.outfd = OpenOutputFile(ToCFile.filename, "w",FTTEXT);
		if (ToCFile.outfd == NULL) {
		    RTFPanic ("Open of %s Failed", ToCFile.filename);
		}
		putwrap("<html><head>", &ToCFile,ToCFile.wrap);
	    putwrap("<!-- This document was created from RTF source by rtftohtml version ", &ToCFile,ToCFile.wrap);
	    putwrap(PVers, &ToCFile,ToCFile.wrap);
	    putwrap(" -->", &ToCFile,ToCFile.wrap);
		putwrap("</head><body>", &ToCFile,ToCFile.wrap);
	}

#ifndef CB_ADD
    putwrap("<html><head>\n", &File1,File1.wrap);
    putwrap("<!-- This document was created from RTF source by rtftohtml version ", &File1,File1.wrap);
    putwrap(PVers, &File1,File1.wrap);
    putwrap(" -->", &File1,File1.wrap);
    putwrap("<html><head>", &Footnote,Footnote.wrap);
    putwrap("<!-- This document was created from RTF source by rtftohtml version ", &Footnote,Footnote.wrap);
    putwrap(PVers, &Footnote,Footnote.wrap);
    putwrap(" -->", &Footnote,Footnote.wrap);
	putwrap("</head><body>", &Footnote,Footnote.wrap);
#else
    if (hierPass) {
	hierFile = File1.outfd;
	hierProlog (File1.outfd);
    }
    else {
    	putwrap("<html><head>\n", &File1,File1.wrap);
	putwrap(hierCreationString (), &File1,File1.wrap);
    }
    if (!fnFileInit) {
    	putwrap("<html><head>\n", &Footnote,Footnote.wrap);
    	putwrap(hierCreationString (), &Footnote,Footnote.wrap);
	putwrap("<title>Footnotes</title></head><body>\n",
		&Footnote,Footnote.wrap);
	fnFileInit = 1;
    }
#endif
}
static void
TestStyle();

HTMLCleanup()
{
    char buff[256];
    	
    /* Fake some plain text so that test-style will undo all current markup. */
    TFont="--Not a Font--";
    TStyle=0;
    TSize=9999;
    ParStyle=PMatchArr[0].PStyle;
    TestStyle(&File1);
#ifndef CB_ADD
	if(anyfoot){	/* We have a footnote */
	    putwrap("</body></html>", &Footnote,0);
		fclose(Footnote.outfd);
	} else {
		fclose(Footnote.outfd);
		remove (Footnote.filename); 
	}
#endif
	if(ToC){
		if(anyTOC){	/* We have a Table of Contents */
		    while(oToCLev>0){
	    		putwrap("</ol>",&ToCFile,0);
	    		oToCLev--;
	    	}
		putwrap("</body></html>", &ToCFile,0);
			fclose(ToCFile.outfd);
		} else {
			fclose(ToCFile.outfd);
			remove (ToCFile.filename); 
		}
	}

#ifdef CB_ADD
    FlushHTML ();
    if (hierPass) {
	hierEpilog (File1.outfd);
        if (hierIndex) {
	    if (!hierIsChangingFile) {
		/* then we've been called by the wrapper */
		generateIndex (0);
		if (hierUseFrames)
		    generateIndex (1);
	    }
	}
    }
    if (!hierPass || !hierIsChangingFile) {
	if(anyfoot){	/* We have a footnote */
	    putwrap("</body></html>", &Footnote,0);
		fclose(Footnote.outfd);
	} else {
		fclose(Footnote.outfd);
		remove (Footnote.filename); 
	}
    }
#endif

    putwrap("</body></html>", &File1,0);
    if (File1.outfd != stdout)
	fclose(File1.outfd);
}

HTMLStartDivert(){
    struct FDest *OD;
	OD = &File1;
	switch (destination) {
    case rtfObjResult:
    	if(linkself&&Item.used>0&&!inTable){
    		sprintf(LineBuffer,"<a href=\"#%s\">",Item.ptr);
    		putwrap(LineBuffer,OD,0);
	    	Item.used=0;
	    }
		break;
	}
}

int BkNum=0;
HTMLEndDivert()
{
    struct FDest *OD;
	OD = &File1;
    if (ISS->destination != destination) {
		switch (destination) {
		case rtfPict:
		    if(PictFile)fclose(PictFile);
		    RTFSetClassCallback(rtfText, PutHTML);
		    break;
		case rtfFootnote:
		    Footnote.NeedPar = 1;
		    Footnote.InCol2 = 0;
		    break;
		case rtfObjResult:
	    	if(linkself){putwrap("</a>",OD,OD->wrap);}
			break;
	    case rtfBookmarkStart:
#ifndef CB_ADD
	    	if(BkName.used>0){
	    		sprintf(LineBuffer,"<a name=\"%s\">",BkName.ptr);
	    		putwrap(LineBuffer,OD,0);
	    		BkName.used=0;
	    	}else {
	    		sprintf(LineBuffer,"<a name=\"RTFtohtmlxx%d\">",BkNum++);
	    		putwrap(LineBuffer,OD,OD->wrap);
	    	}
#endif
			break;
	    case rtfBookmarkEnd:
#ifndef CB_ADD
	    	putwrap("</a>",OD,OD->wrap);
#endif
			break;
	    }
	}
	if(ISS->ToCLev != ToCLev&&ToC&&ToCLev>0){
	    	putwrap("</a>",&ToCFile,0);
	    	putwrap("</a>",OD,0);
	    	ToCText=0;
	}

}
int xval=0;
int charnum = 0;
extern int csTop;
void
ReadPict(){
	int mval;
	int level=0;
	int savecsTop;
	if(csTop>0)savecsTop=csTop-1;
	else savecsTop=0;

	while (1)
	{
		if (rtfClass == rtfEOF)
			break;
		if (rtfClass == rtfGroup)
		{
			if (rtfMajor == rtfBeginGroup)
				++level;
			else if (rtfMajor == rtfEndGroup)
			{
				if (--level < 1)
					break;	/* end of initial group */
			}
		} else {
			if(rtfMajor != '\n' && rtfMajor != '\r'
							&& rtfMajor != '\0'){
				if(rtfMajor <= '9'){
					mval=rtfMajor-'0';
				} else if(rtfMajor <= 'F'){
					mval=rtfMajor-'A'+10;
				} else {
					mval=rtfMajor-'a'+10;
				} 
			    if (++charnum == 2) {
			    	xval=xval*16+mval;
					fputc((char) xval, PictFile);
					charnum = 0;
			    } else {
			    	xval=mval;
			    }
			}
		}
		_RTFGetToken2();
	}
	csTop=savecsTop;
}


/*
 * This function "cheats" by using rtfMajor (raw input character) rather
 * than rtfMinor (corresponding standard character code).  This is based on
 * the assumption than the function won't see anything other than
 * '0'..'9' and that the result of mapping the standard codes for such chars
 * would be the same chars.
 */

static void
PutHex()
{
	int mval;
	if(rtfMajor <= '9'){
		mval=rtfMajor-'0';
	} else if(rtfMajor <= 'F'){
		mval=rtfMajor-'A'+10;
	} else {
		mval=rtfMajor-'a'+10;
	} 
    if (++charnum == 2) {
    	xval=xval*16+mval;
		fputc((char) xval, PictFile);
		charnum = 0;
    } else {
    	xval=mval;
    }
}

char zeros[512];

/*
	A placeable Windows metafile is a standard Windows metafile that has an 
	additional 22-byte header. The header contains information about the aspect 
	ratio and original size of the metafile, permitting applications to display 
	the metafile in its intended form. The header for a placeable Windows metafile 
	has the following form: 
typedef struct {
    DWORD   key;
    HANDLE  hmf;
    RECT    bbox;
    WORD    inch;
    DWORD   reserved;
    WORD    checksum;
} METAFILEHEADER;
NOTE: DWORD=4bytes, HANDLE=2,WORD=2,RECT=8.
(Apparently the IBM does byte swapping?)
Following are the members of a placeable metafile header: 
key			Specifies the binary key that uniquely identifies this file type. 
			This member must be set to 0x9AC6CDD7L. 
hmf			Unused; must be zero. 
bbox		Specifies the coordinates of the smallest rectangle that encloses 
			the picture. The coordinates are in metafile units as defined by the inch 
			member. *** the first two points are fixed, the second two are
			the height and width ***
inch		Specifies the number of metafile units to the inch. To avoid numeric 
			overflow, this value should be less than 1440. 
			Most applications use 576 or 1000.  *** we use 1440 ***
reserved	Unused; must be zero. 
checksum	Specifies the checksum. It is the sum (using the XOR operator) of the 
			first 10 words of the header. 

*/
unsigned char wmfhead[22]={
/* key = */ 0xd7,0xcd,0xc6,0x9a,
/* hmf = */ 0,0,
/* bbox = */ 0xfc,0xff,0xfc,0xff, /*width*/0,0,/*height*/0,0,
/* inch = */ 0xa0,0x05,
/* reserved = */ 0,0,0,0,
/* checksum = */ 0,0};

void
NabPicture()
{
    struct FDest *OD;
    /*
     * First create the reference to the picture, and then change the class
     * callback so that the contents of the picture are written to a file. If
     * we are already gathering a link, or the -i option was specified,
     * change the format of the link to IMG.
     */
    char filename[256];
    char rfilename[256];
    char reftext[512];
#ifdef NOTDEF
    if(inTable){
		RTFMsg ("Error, Graphics not allowed in tables - skipping\n");
		context();
    	RTFSkipGroup ();
		PopIState();
		return;
    }
#endif
    sprintf(filename, "%s%d.%s", FPrefix, ++outfuniq, PictExt);
    sprintf(rfilename, "%s%d.%s", FPrefixR, outfuniq, PFileExt);

    /* find the previous destination */
    if (ISS == NULL) {
		RTFMsg ("Error, State Stack is null\n");
	    context();
	exit(1);
    }
    switch (ISS->destination) {
	/* string destinations */
    case rtfFootnote:
	OD = &Footnote;
	break;
    default:
	OD = &File1;
	break;
    }
    if(PNText){	/* We have saved ParNumText */
    	PNText=0;
    	PlayST(&PNSave);
    }
    if(FNText==1){
	    	FNText=2;
	    	PlayST(&FNSave);
	    	FNText=0;
    }
    if(inTable){
	    sprintf(LineBuffer,"<IMG SRC=\"%s\">", rfilename);
		AppendText(&(cell[cellno].cbuff), LineBuffer);
	}else {
	    if (IStyle_Chg)	TestStyle(OD);
	    if (OD->TSpecial == MTDiscard){
			RTFSkipGroup ();
			PopIState();
			return;
	    }
	    if (IMG || OD->TSpecial == MTHot) {
	    	sprintf(LineBuffer,"<IMG SRC=\"%s\">", rfilename);
	    } else {
		    sprintf(LineBuffer,"<A href=\"%s\">Click here for Picture </A>", rfilename);
	    }
	    if (OD->NeedPar) {
			DoPmark(OD);
			OD->NeedPar = 0;
	    }
	    putwrap(LineBuffer,OD,0);
	}
	if(WriteGraf){
	
	    PictFile = OpenOutputFile(filename, "wb",PictType);
	    if (PictFile == NULL) {
			RTFPanic ("Open of %s Failed", filename);
	    }
	    if (PictType == FTPICT) {
		fwrite(zeros, 512, 1, PictFile);
   		} else if (PictType == FTWMF ) {
   			int i;
   			/* write a WMF header for a placeable Windows Metafile. */
   			/* Calculate Width and Heigth of bounding box */
   			wmfhead[10]=(PicGoalWid-4)%256;
   			wmfhead[11]=(PicGoalWid-4)/256;
   			wmfhead[12]=(PicGoalHt-4)%256;
   			wmfhead[13]=(PicGoalHt-4)/256;
   			/* compute Checksum */
   			wmfhead[20]=0;
   			wmfhead[21]=0;
   			for(i=0;i<20;i+=2){
   				wmfhead[20]^=wmfhead[i];
   				wmfhead[21]^=wmfhead[i+1];
   				}
   			fwrite(wmfhead,22,1,PictFile);
   		}
   		ReadPict();
   		RTFSetClassCallback (rtfText, PutHTML);
   		PopIState();
    } else {
		RTFSkipGroup ();
		PictFile=NULL;
		PopIState();
    }
}




static void
TestStyle(OD)
    struct FDest *OD;
{
/*
	Change the style of the output text to the new style.
	If the style changes, force correct nesting of HTML tags.
*/
    int PMidx, i, j,k;
    int OldPMidx;
    int OldTSpecial = OD->TSpecial;
	if (!InBody) {
	    /*
	     * If we get here, we have text destined for the body. close out
	     * the head section.
	     */
#ifndef CB_ADD
	    if (Title.used) {
#else
	    if (Title.used && !hierPass) {
#endif
		sprintf(LineBuffer,"<title>%s</title>", Title.ptr);
		putwrap(LineBuffer, &File1,1);
	    }
#ifndef CB_ADD
	    putwrap("</head><body>", &File1,1);
#else
	    if (!hierPass)
	    putwrap("</head><body>", &File1,1);
#endif
	    InBody = 1;
	}
	
    if(OD->HPTags<0)
    	OldPMidx = -1;
    else
    	OldPMidx = OD->TStack[OD->HPTags];
    /* Look for Special Text Style Matches - these override normal processing */
    IStyle_Chg = 0;
    OD->TSpecial = 0;
    
    
    /* Find a match on the new paragraph and text styles */
    PMidx = PMatchLen;		/* to catch a null ParStyle */
    for(k=0;ParStyle[k]!='\0';k+=i){/* for each alias in ParStyle */
    	/* find the size of the ParStyle alias */
    	for(i=0;ParStyle[k+i]!='\0'&&ParStyle[k+i]!=',';i++);
	    for (PMidx = 0; PMidx < PMatchLen; PMidx++) {
			if (strncmp(PMatchArr[PMidx].PStyle, ParStyle,i) == 0) {
			    break;
		    }
		}
		if (PMidx != PMatchLen)break;
	}

    if (PMidx == PMatchLen){
		PMidx = 0;		/* The first entry is the default */
		if(strcmp(ParStyle,"")!=0){
			for(i=0;i<256&&badstyles[i]!=NULL;i++) /* Save off mismatches */
				if(strcmp(badstyles[i],ParStyle)==0)
					break;
			if(i<256&&badstyles[i]==NULL){
				badstyles[i]=ParStyle;
				RTFMsg ("Unknown Paragraph style: %s\n",ParStyle);
			}
		}
	}

    
    
    
	if(PMatchArr[PMidx].PTidx>MTSPECIAL&&
    	(PMatchArr[PMidx].PTidx != MTHref || OD != &Footnote)){
    	OD->TSpecial=PMatchArr[PMidx].PTidx;
    } else {
	    for (i = 0; i < TMatchLen; i++) {
			if ((TMatchArr[i].TStyle == (TMatchArr[i].TMask & TStyle)) &&
			    (TMatchArr[i].Font[0] == '\0' || strcmp(TMatchArr[i].Font, TFont) == 0) &&
			    (TMatchArr[i].FSize == 0 || TMatchArr[i].FSize == TSize)) {
			    /* can't have hot text in footnote */
			    if (TMatchArr[i].TTidx == MTHref && OD == &Footnote)
					continue;
			    if (TMatchArr[i].TTidx > MTSPECIAL) {
					OD->TSpecial = TMatchArr[i].TTidx;
					break;
			    }
			}
	    }
    }
    if (inTable){ /* force specials off */
    	OD->TSpecial=0;
    	ParStyle="_Table";
    }
	if (FNMatch.TStyle == (TStyle) &&
	    strcmp(FNMatch.Font, TFont) == 0 &&
	     FNMatch.FSize == TSize) {
			OD->TSpecial = MTFootNote;
	}
    if (OldTSpecial != OD->TSpecial) {
	    switch(OldTSpecial){
	    case MTName:
		    putwrap("\"> </a>", OD,0);
			OD->wrap=PTagArr[OD->PTidx].DoFold;
	    	break;
	    case MTHref:
#ifndef CB_ADD
		    putwrap("\">", OD,0);
#else
		    if (hierPass) {
			if (*anchorBuf) {
			    putwrap (FileOfAnchor (anchorBuf), OD, 0);
			    putwrap (anchorBuf, OD, 0);
			}
			putwrap ("\">", OD, 0);
		    } else
		    putwrap("\">", OD,0);
#endif
		    if(OD->TSpecial!=MTHot){
		    	RTFMsg ("Error, at end of href, hot text not found\n");
	    		context();
	    		putwrap("</a>", OD,0);
		    }
			OD->wrap=PTagArr[OD->PTidx].DoFold;
	    	break;
	    case MTHot:
			putwrap("</a>",OD,OD->wrap);
	    	break;
	    case MTFootNote:
			if (OD == &File1) {
				putwrap("]</a>", OD,OD->wrap);
				FNMatch.Font= "NotAFont";
			} else {
				putwrap("]</a>",OD,OD->wrap);
			}
	    	break;
	    }
    }
	if (OD->TSpecial != 0){
  		if (OldTSpecial != OD->TSpecial) {
			while (OD->HTTags > OD->HPTags) {
			    putwrap(TTagArr[TMatchArr[OD->TStack[OD->HTTags]].TTidx].EndTag,
				OD,OD->wrap);
			    OD->HTTags--;
			}
		    switch(OD->TSpecial){
		    case MTName:
		    	if (OD->NeedPar) {
					putwrap(PTagArr[OD->PTidx].ParTag, OD,OD->wrap);
					OD->NeedPar = 0;
		   		}
				putwrap("<a name=\"", OD,0);
				OD->wrap=0;
		   		break;
		    case MTHref:
		    	if (OD->NeedPar) {
					putwrap(PTagArr[OD->PTidx].ParTag, OD,OD->wrap);
					OD->NeedPar = 0;
		   		}
				putwrap("<a href=\"", OD,0);
				OD->wrap=0;
#ifdef CB_ADD
				anchorBuf[0] = '\0';
#endif
		   		break;
		    case MTHot:
			    if(OldTSpecial!=MTHref){
			    	RTFMsg ("Error, hot text found without preceding href\n");
	   				context();		  			
			    }		    	
				break;
		    case MTFootNote:
				if (OD->NeedPar) {
					putwrap(PTagArr[OD->PTidx].ParTag, OD,OD->wrap);
					OD->NeedPar = 0;
				}
				if (OD == &File1) {
					anyfoot=1;
#ifndef CBADD					
					sprintf(LineBuffer,"<a href=\"%s_fn.html#fn%d\">[", FPrefixR,uniqnum);
#else
					if (!hierShortFileNames)
					sprintf(LineBuffer,"<a href=\"%s_fn.html#fn%d\">[", FPrefixR,uniqnum);
					else
					    sprintf(LineBuffer,"<a href=\"%sfn.htm#fn%d\">[", FPrefixR,uniqnum);
#endif					
					putwrap(LineBuffer,OD,OD->wrap);
				} else {
					sprintf(LineBuffer,"<a name=\"fn%d\">[", uniqnum++);
					putwrap(LineBuffer,OD,OD->wrap);
				}
				break;
		    }
		}
		return;
	}

    OD->PTidx = PMatchArr[PMidx].PTidx;
    if (PMidx != OldPMidx) { /* Paragraph Style changed */
    	int pushpop=0;
		/* Pop all of the Text Styles */
		while (OD->HTTags > OD->HPTags) {
		    putwrap(TTagArr[TMatchArr[OD->TStack[OD->HTTags]].TTidx].EndTag,
			OD,OD->wrap);
		    OD->HTTags--;
		}
		if (PMatchArr[PMidx].NestLev > OD->HPTags + 1) {
		    RTFMsg ("Error, found style:%s at a nesting level of %d\n"
			,ParStyle, OD->HPTags);
		    context();
		}
		while (OD->HPTags >= PMatchArr[PMidx].NestLev) {
		    /* Pop Paragraph Tags  */
		    if(OD->TStack[OD->HPTags] == PMidx){
		    	pushpop=1;
		    	break;
		    } else {
			    if (strcmp(PTagArr[PMatchArr[OD->TStack[OD->HPTags]].PTidx].EndTag, "") != 0
			    && strcmp(PTagArr[OD->PTidx].StartTag, "") == 0) {
				/* if there was a closing tag, we no longer need a paragraph */
				OD->NeedPar = 0;
			    }
			    putwrap(PTagArr[PMatchArr[OD->TStack[OD->HPTags]].PTidx].EndTag, OD,
			    PTagArr[PMatchArr[OD->TStack[OD->HPTags]].PTidx].DoFold);
			    OD->HPTags--;
			}
		}
		if(!pushpop){
			if(OD->HPTags >= 0&&!PTagArr[PMatchArr[OD->TStack[OD->HPTags]].PTidx].CanNest) {
			    /* Nesting Error */
			    RTFMsg ("Error, can't use style:%s within %s\n",
				ParStyle,
				PMatchArr[OD->TStack[OD->HPTags]].PStyle);
			    context();
			}
			/* Now push the new style */
			OD->HPTags++;
			OD->TStack[OD->HPTags] = PMidx;
			if (strcmp(PTagArr[OD->PTidx].StartTag, "") != 0) {
			    /* if there is a Starting tag, we no longer need a paragraph */
			    putwrap(PTagArr[OD->PTidx].StartTag, OD,PTagArr[OD->PTidx].DoFold);
			    OD->NeedPar = 0;
			}
		}
		OD->HTTags = OD->HPTags;
  		OD->wrap=PTagArr[OD->PTidx].DoFold;
	}
    if (OD->NeedPar) {
		DoPmark(OD);
		OD->NeedPar = 0;
    }
  	if(ToCLev==0){
			 HToCLev=PTagArr[PMatchArr[PMidx].PTidx].ToCLev;
		} else {
			 HToCLev=ToCLev;
		}

    /* Check remaining Text Styles to see if they should be popped */
    for (i = OD->HPTags + 1; i <= OD->HTTags; i++) {
	if ((TMatchArr[OD->TStack[i]].TStyle != (TMatchArr[OD->TStack[i]].TMask & TStyle)) ||
	    (TMatchArr[OD->TStack[i]].Font[0] != '\0' && strcmp(TMatchArr[OD->TStack[i]].Font, TFont) != 0) ||
	    (TMatchArr[OD->TStack[i]].FSize != 0 && TMatchArr[OD->TStack[i]].FSize != TSize)) {
	    while (OD->HTTags >= i) {
		putwrap(TTagArr[TMatchArr[OD->TStack[OD->HTTags]].TTidx].EndTag,
		    OD,OD->wrap);
		OD->HTTags--;
	    }
	}
    }
    /* Now check all Text Styles and see if anything else must be pushed */
    if (PTagArr[PMatchArr[OD->TStack[OD->HPTags]].PTidx].AllowText) {
	for (i = 0; i < TMatchLen; i++) {
	    if ((TMatchArr[i].TStyle == (TMatchArr[i].TMask & TStyle)) &&
		(TMatchArr[i].Font[0] == '\0' || strcmp(TMatchArr[i].Font, TFont) == 0) &&
		(TMatchArr[i].FSize == 0 || TMatchArr[i].FSize == TSize)) {
		for (j = OD->HPTags + 1; j <= OD->HTTags; j++) {
		    if (TMatchArr[OD->TStack[j]].TTidx == TMatchArr[i].TTidx)
			break;
		}
		if (j > OD->HTTags) {
		    /* didn't find it - push it on */
		    OD->HTTags++;
		    OD->TStack[OD->HTTags] = i;
		    if (TMatchArr[i].TTidx >= TTagLen) {
		    	RTFMsg ("Fatal Error");
	    		context();
		    	RTFPanic ("Internal trap - tried to push special: %d\n",TMatchArr[i].TTidx);
		    }
		    putwrap(TTagArr[TMatchArr[i].TTidx].StartTag,
			OD,OD->wrap);
		}
	    }
	}
    }			
}

DoPmark(OD)
    struct FDest *OD;
{
	/* pop all text styles, print the paragraph mark and push them back */
	int i;
	OD->NeedPar=0;
	for(i=OD->HTTags;i>OD->HPTags;i--){
	    putwrap(TTagArr[TMatchArr[OD->TStack[i]].TTidx].EndTag,
		OD,OD->wrap);
	}
	putwrap(PTagArr[OD->PTidx].ParTag, OD,OD->wrap);
	for(i=OD->HPTags+1; i<=OD->HTTags;i++){
	    putwrap(TTagArr[TMatchArr[OD->TStack[i]].TTidx].StartTag,
		OD,OD->wrap);
	}
}

PutSpecial(Special)
    int Special;
{
    int i,j,n,opos,dataleft;
    char orec[MAXOBUF];
    struct FDest *OD;
    switch (destination) {
	/* string destinations */
    case rtfITitle:
    case rtfInfo:
    case rtfISubject:
    case rtfIAuthor:
    case rtfIOperator:
    case rtfIKeywords:
    case rtfIComment:
    case rtfIVersion:
    case rtfIDoccomm:
	return;
	case rtfParNumText:
    	SaveSt(&PNSave,1+Special);
    	PNText=1;
		return;
	/* file destinations */
    case rtfHeader:
    case rtfHeaderLeft:
    case rtfHeaderRight:
    case rtfHeaderFirst:
    case rtfFooter:
    case rtfFooterLeft:
    case rtfFooterRight:
    case rtfFooterFirst:
	return;
    case rtfFootnote:
	OD = &Footnote;
	break;
    default:
	OD = &File1;
	break;
    }
    if(PNText){	/* We have saved ParNumText */
    	PNText=0;
    	PlayST(&PNSave);
    }
    if(FNText==1){
	    	FNText=2;
	    	PlayST(&FNSave);
		    FNText=0;
    }
    if (IStyle_Chg){
	    if(inTable){
			TFont="--Not a Font--";
	    	TStyle=0;
	   		TSize=9999;
	   		ParStyle="_Table";
	    }
    	TestStyle(OD);
    }
    if (OD->NeedPar) {
		DoPmark(OD);
		OD->NeedPar = 0;
    }
	switch (Special) {
    case rtfRow:	/* Clear out all saved cell data */
    	dataleft=1;
	    for(i=firstcell;i<lastcell;i++)
	    	cell[i].cpos=0;
    	while(dataleft){
    		dataleft=0;
    		opos=0;
	    	for(i=firstcell;i<lastcell;i++){
	    		if(cell[i].merge||cell[i].cpos>=cell[i].cbuff.used){
	    			for(j=0;j<=cell[i].width;j++){
	    				orec[opos++]=' ';
	    			}
	    			continue;
	    			}
	    		cell[i].cpos+=nextfield(&(cell[i].cbuff.ptr[cell[i].cpos]), 
	    			cell[i].width-1,  &(orec[opos]), cell[i].just,1);
	    		/* skip leading blanks */
	    		while(cell[i].cbuff.ptr[cell[i].cpos]==' '||cell[i].cbuff.ptr[cell[i].cpos]=='\t')
	    			cell[i].cpos++;
	    		n=strlen(&(orec[opos]));
	    		orec[opos+n]=' ';
	    		orec[opos+n+1]=' ';
	    		opos+=n+2; 		
	    		if(cell[i].cpos<cell[i].cbuff.used)
	    			dataleft=1;
	    	}
    		orec[opos++]='\n';
    		orec[opos]='\0';
    		putwrap(orec, OD,0);
    	}
	    for(i=firstcell;i<lastcell;i++)
	    	cell[i].cbuff.used=0;
		break;
    case rtfPar:
    	if(inTable){
    		AppendText(&(cell[cellno].cbuff), " ");
			return;
    	}   		
		if (OD->TSpecial == MTDiscard)
		    return;
    	if (OD->InCol2==0 && PTagArr[OD->PTidx].DeleteCol1==1) { 
    		/* We deleted Everything */
	    	if(SkipCol.used>0){
	    		RTFMsg ("Warning - required Tab missing from list entry - recovering text");
		    	context();
	    		putwrap(SkipCol.ptr,OD,OD->wrap);
    		}
    	}
		OD->NeedPar = 1;
		OD->InCol2 = 0;
	    SkipCol.used=0;
	    if(ToC&&ToCText&&OD==&File1&&HToCLev>0){
	    	putwrap("</a>",&ToCFile,0);
	    	putwrap("</a>",OD,0);
	    	ToCText=0;
	    }
		break;
    case rtfTab:
    	if(inTable){
    		AppendText(&(cell[cellno].cbuff), " ");
			return;
    	}   		
		if (OD->TSpecial == MTDiscard)
		    return;
		if (!OD->InCol2) {
		    putwrap(PTagArr[OD->PTidx].Col2Tag, OD,OD->wrap);
	    	SkipCol.used=0;
		    OD->InCol2 = 1;
		} else {
		    putwrap(PTagArr[OD->PTidx].TabTag, OD,OD->wrap);
		}
		break;
    }
}



context(){
	int cptr;
	char ctxbuff[256];
	sprintf (ctxbuff,"At line %ld, position %d.\n",rtfLineNum, rtfLinePos);

	RTFMsg (ctxbuff);
}

void
PutHTMLString(p)
    char *p;
{
    int i, j;
    struct FDest *OD;
    switch (destination) {
	/* string destinations */
    case rtfITitle:
	AppendText(&Title, p);
	return;
#ifdef CB_ADD
    case rtfIndex:
        if (hierPass && hierIndex)
	   strcat (indexEntry, p);
	return;
#endif
    case rtfInfo:
    case rtfISubject:
    case rtfIAuthor:
    case rtfIOperator:
    case rtfIKeywords:
    case rtfIComment:
    case rtfIVersion:
    case rtfIDoccomm:
	return;
	case rtfParNumText:
		AppendText(&PNSave.SD, p);
	    if (IStyle_Chg)SaveSt(&PNSave,0);
	    PNText=1;
		return;
    case rtfObjItem:
	AppendText(&Item, p);
	return;
    case rtfBookmarkStart:
	AppendText(&BkName, p);
	return;
    case rtfBookmarkEnd:
	return;
	/* file destinations */
    case rtfHeader:
    case rtfHeaderLeft:
    case rtfHeaderRight:
    case rtfHeaderFirst:
    case rtfFooter:
    case rtfFooterLeft:
    case rtfFooterRight:
    case rtfFooterFirst:
	return;
    case rtfFootnote:
	OD = &Footnote;
	break;
    default:
	OD = &File1;
	break;
    }	
    if(PNText){	/* We have saved ParNumText */
    	PNText=0;
    	PlayST(&PNSave);
    }
    if(inTable){
		AppendText(&(cell[cellno].cbuff), p);
		return;
	}
    if(FNText==1){
	    	FNText=2;
	    	PlayST(&FNSave);
		    FNText=0;
    } else if(FNText==0&&IStyle_Chg&&destination!=rtfFootnote) { /* We have a candidate footnote */
    	FNText=1;
    	AppendText(&FNSave.SD, p);
	    SaveSt(&FNSave,0);
    	return;
    }
    if (IStyle_Chg){
    	TestStyle(OD);
    }
    if (OD->TSpecial == MTDiscard)
	return;
    if (OD->InCol2==0 && PTagArr[OD->PTidx].DeleteCol1==1){
    	AppendText(&SkipCol, p);
		return;
	}
    if (OD->NeedPar) {
		DoPmark(OD);
		OD->NeedPar = 0;
    }
    /* If we are looking at <,> or & that have been translated
    	and we are in MHLiteral, untranslated them.
    */
    if (OD->TSpecial == MTLiteral){
    	if(strcmp(p,"&lt;")==0)p="<";
    	else if(strcmp(p,"&gt;")==0)p=">";
    	else if(strcmp(p,"&amp;")==0)p="&";
    } else if ((ConvertCaps) && p[0] != '&' && p[0] != '[') {
    /* do uppercasing if string doesn't begin with '&' or '[' */
	char *s = p;

	while (*s != '\0') {
	    if (islower(*s))
		*s = toupper(*s);
	    ++s;
	}
    }
    if(ToC&&OD==&File1&&HToCLev>0){
    	while(oToCLev>HToCLev){
    		putwrap("</ol>",&ToCFile,0);
    		oToCLev--;
    	}
    	while(oToCLev<HToCLev){
    		putwrap("<ol>",&ToCFile,0);
    		oToCLev++;
    	}
    	if(ToCText==0){
    	    anyTOC=1;
    		sprintf(LineBuffer,"\n<li><a href=\"%s.html#RTFToC%d\">", FPrefixR,ToCidx);
    		putwrap(LineBuffer,&ToCFile,0);
    		sprintf(LineBuffer,"<a name=\"RTFToC%d\">",ToCidx++);
    		putwrap(LineBuffer,OD,0);
    		ToCText=1;
    	}
    	putwrap(p,&ToCFile,0);
    }

    putwrap(p, OD,OD->wrap);
}

void
PutHTML()
{
    char *p;

#ifndef CB_ADD
    p = MapChar(rtfMinor);
    PutHTMLString(p);
#else
    p = MapChar(rtfMinor);
    if (hierPass) {
        if (inHeading) {
	    return;
	} else {
	    hierHasText = 1;
	    if (inHeadingRef) {
	       strcat (headingRef, MapChar(rtfMinor));
	       return;
	    }
	}
    }
    if (Center == 1) {
	Center = 2;
	hierPrint ("<center>");
    }

    PutHTMLString(p);
#endif
}


#ifdef CB_ADD
void FlushHTML ()
{
  if (File1.NeedPar)
    DoPmark (&File1);	/* Phew... now I got it. */
  if (File1.cpos > 0) {
      File1.wbuff[File1.cpos] = '\0';
      fputs (File1.wbuff,File1.outfd);
  }
  File1.cpos= -1;
}

void UndoMarkup ()
{
  /* Fake some plain text so that test-style will undo all
	 current markup. */
  TFont="--Not a Font--";
  TStyle=0;
  TSize=9999;
  ParStyle=PMatchArr[0].PStyle;
  TestStyle(&File1);
}

void hierPrint (str) 
char *str;
{
	if (File1.NeedPar)
	    DoPmark (&File1);
	putwrap (str, &File1, File1.wrap);
}

void resetParNumText ()
{
    PNSave.SD.used = 0;
    PNSave.used = 0;
    PNText = 0;
}
#endif


/*
 * Map a standard character code to an output string.
 * Returns pointer to static string.
 */

#ifndef CB_ADD
static char *
#else
char * 		/* hier.c needs this! */
#endif
MapChar(stdCode)
    int stdCode;
{
    static char buf[100];
    char *oStr;

    if (stdCode == rtfSC_nothing){
    sprintf(buf,"&#%d;",rtfMajor);
	RTFMsg("Invalid Character Code \\`%x - outputting &#%d;\n",rtfMajor,rtfMajor);
    }else {
	oStr = outMap[stdCode];
	if (oStr == (char *) NULL) {	/* no output sequence in map */
	    sprintf(buf, "[[%s]]", RTFStdCharName(stdCode));
	    RTFMsg("No output translation for :%s\n",RTFStdCharName(stdCode));
	} else
	    (void) strcpy(buf, oStr);
    }
    return (buf);
}


static void
AppendText(OD, text)
    struct SDest *OD;
    char *text;
{
    int need;
    char *ptr;

    need = OD->used + 1 + strlen(text);	/* need space for next char + '\0' */
    if (OD->alloc < need) {
	OD->alloc += DSALLOC * (1 + (need / DSALLOC));
	ptr = (char *) RTFAlloc(
	    (int) sizeof(char) * OD->alloc);
	if (ptr == NULL) {
	    RTFMsg ("Memory Allocation Failed\n");
	    context();
	   RTFPanic ("");
	}
	if (OD->ptr != NULL) {
	    (void) strcpy(ptr, OD->ptr);
	    RTFFree(OD->ptr);
	}
	OD->ptr = ptr;
    }
    strcpy(&(OD->ptr[(OD->used)]), text);
    OD->used += strlen(text);
}

/* 
	Need to save off  Special Character (int)
	and current text position
	and current save state pointer
*/
SaveSt(SS,Special)
struct SSStruct *SS;
{
	struct SSData *ptr;
	int i;
	if(SS->alloc==SS->used){
		SS->alloc+=40;
		ptr = (struct SSData *) RTFAlloc(
	    (int) sizeof(struct SSData) * SS->alloc);
		if (ptr == NULL) {
		    RTFMsg ("Memory Allocation Failed\n");
		    context();
		   RTFPanic ("");
		}
		if(SS->data != NULL) {
			for(i=0;i<SS->used;i++){
				ptr[i]=SS->data[i];
			}
			RTFFree(SS->data);
		}
		SS->data=ptr;
	}
	SS->data[SS->used].Special=Special;
	if(IStyle_Chg){
		SS->data[SS->used].TheState=SaveIState();
	} else {
		SS->data[SS->used].TheState=NULL;
	}
	SS->data[SS->used++].cpos=SS->SD.used;
}
PlayST(SS)
struct SSStruct *SS;
{
	int i,tidx=0;
	InStateStack *SavedState;
	char savchar;
	SavedState=SaveIState();
	for(i=0;i<SS->used;i++){
		if(SS->data[i].TheState){
			RestoreIState(SS->data[i].TheState);
			SS->data[i].TheState=NULL;
			if(destination==rtfParNumText){
			    destination=SavedState->destination;
			    ParStyle=SavedState->ParStyle;
		    } else if(destination!=rtfFootnote &&
		    	SavedState->destination==rtfFootnote){
				FNMatch.TStyle = TStyle;
				FNMatch.Font= TFont;
				FNMatch.FSize = TSize;
		    }
		}
		/* First dispense with all leading text */
		if(tidx < SS->SD.used){
			savchar=SS->SD.ptr[SS->data[i].cpos];
			SS->SD.ptr[SS->data[i].cpos]='\0';
			PutHTMLString(&(SS->SD.ptr[tidx]));
			tidx=SS->data[i].cpos;
			SS->SD.ptr[SS->data[i].cpos]=savchar;
		}
		if(SS->data[i].Special){
			PutSpecial(SS->data[i].Special-1);
		}
	}
	SS->SD.used=0;
	SS->used=0;
	RestoreIState(SavedState);
}
