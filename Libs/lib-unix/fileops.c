/*
 * fileops.c -- file operations for UNIX versions of translators.
 */
/*
 * UnixOpenLibFile () - Open a library file.  Looks in the following
 * directories:
 * - Looks in current directory.
 * - if RTFLIBDIR environment variable is set, looks in directory named by it.
 * - Looks in executable program's directory, if UnixSetProgPath() has
 * been called.
 * - Looks in library directory, LIBDIR.
 *
 * Exception: if file is an absolute pathname, look only for file as named.
 *
 * Returns NULL if file cannot be found and opened.
 */
 
# include	<stdio.h>
 
# include	"rtf.h"
# include	"rtf-unix.h"
 
extern char	*getenv ();
 
static char	*progPath = (char *) NULL;
 
 
void
UnixSetProgPath (path)
char	*path;
{
int	i, j, n;

	n = strlen (path);
	for (j = -1, i = 0; i < n; i++)
	{
		if (path[i] == '/')
			j = i;
	}
	if (j < 0)		/* no slash found */
	{
		path = ".";
		j = 1;
	}
	if ((progPath = RTFAlloc (j + 1)) != (char *) NULL)
	{
		(void) strncpy (progPath, path, j);
		progPath[j] = '\0';
	}
}
 
 
FILE *
UnixOpenLibFile (file, mode)
char	*file;
char	*mode;
{
FILE	*f;
char	buf[rtfBufSiz];
char	*p;
 
	if ((f = fopen (file, mode)) != (FILE *) NULL)
	{
		return (f);
	}
	/* if abolute pathname, give up, else look in library */
	if (file[0] == '/')
	{
		return ((FILE *) NULL);
	}
	if ((p = getenv ("RTFLIBDIR")) != (char *) NULL)
	{
		sprintf (buf, "%s/%s", p, file);
		if ((f = fopen (buf, mode)) != (FILE *) NULL)
			return (f);
	}
	if (progPath != (char *) NULL)
	{
		sprintf (buf, "%s/%s", progPath, file);
		if ((f = fopen (buf, mode)) != (FILE *) NULL)
			return (f);
	}
	sprintf (buf, "%s/%s", "g:/rtftohtml-2.7.5", file);
	f = fopen (buf, mode);	/* NULL if it fails */
	return (f);
}
