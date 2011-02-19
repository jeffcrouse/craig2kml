/*
 @(#)File:           $RCSfile: mkpath.c,v $
 @(#)Version:        $Revision: 1.12 $
 @(#)Last changed:   $Date: 2008/05/19 00:43:33 $
 @(#)Purpose:        Create all directories in path
 @(#)Author:         J Leffler
 @(#)Copyright:      (C) JLSS 1990-91,1997-98,2001,2005,2008
 @(#)Product:        :PRODUCT:
 */
#include "mkpath.h"

#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <string.h>
#include <sys/stat.h> 

typedef struct stat Stat;


int do_mkdir(const char *path, mode_t mode)
{
    Stat            st;
    int             status = 0;
	
    if (stat(path, &st) != 0)
    {
        /* Directory does not exist */
        if (mkdir(path, mode) != 0)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = -1;
    }
	
    return(status);
}

/**
 ** mkpath - ensure all directories in path exist
 ** Algorithm takes the pessimistic view and works top-down to ensure
 ** each directory in path exists, rather than optimistically creating
 ** the last element and working backwards.
 */
int mkpath(const char *path, mode_t mode)
{
    char           *pp;
    char           *sp;
    int             status;
    char           *copypath = strdup(path);
	
    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(path, mode);
    free(copypath);
    return (status);
}

#ifdef TEST

#include <stdio.h>

int main(int argc, char **argv)
{
    int             i;
	
    for (i = 1; i < argc; i++)
    {
        if (mkpath(argv[i], 0777) == 0)
            printf("created: %s\n", argv[i]);
        else
            printf("failed to create: %s\n", argv[i]);
    }
    return(0);
}

#endif /* TEST */