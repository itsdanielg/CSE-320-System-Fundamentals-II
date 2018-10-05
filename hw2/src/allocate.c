
/*
 * Allocate storage for the various data structures
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "global.h"
#include "gradedb.h"
#include "stats.h"
#include "allocate.h"
#include "error.h"

char *memerr = "Unable to allocate memory.";

Professor *newprofessor()
{
        Professor *p;
        if((p = malloc(sizeof(Professor))) == NULL)
                fatal(memerr);
        return(p);
}

Assistant *newassistant()
{
        Assistant *a;
        if((a = malloc(sizeof(Assistant))) == NULL)
                fatal(memerr);
        return(a);
}

Student *newstudent()
{
        Student *s;
        if((s = malloc(sizeof(Student))) == NULL)
                fatal(memerr);
        return(s);
}

Section *newsection()
{
        Section *s;
        if((s = malloc(sizeof(Section))) == NULL)
                fatal(memerr);
        return(s);
}

Assignment *newassignment()
{
        Assignment *a;
        if((a = malloc(sizeof(Assignment))) == NULL)
                fatal(memerr);
        return(a);
}

Course *newcourse()
{
        Course *c;
        if((c = malloc(sizeof(Course))) == NULL)
                fatal(memerr);
        return(c);
}

Score *newscore()
{
        Score *s;
        if((s = malloc(sizeof(Score))) == NULL)
                fatal(memerr);
        return(s);
}

char *newstring(tp, size)
char *tp;
int size;
{
        char *s, *cp;
        if((s = malloc(size)) == NULL)
                fatal(memerr);
        cp = s;
        while(size-- > 0) *cp++ = *tp++;
        return(s);
}

Freqs *newfreqs()
{
        Freqs *f;
        if((f = malloc(sizeof(Freqs))) == NULL)
                fatal(memerr);
        return(f);
}

Classstats *newclassstats()
{
        Classstats *c;
        if((c = malloc(sizeof(Classstats))) == NULL)
                fatal(memerr);
        return(c);

}

Sectionstats *newsectionstats()
{
        Sectionstats *s;
        if((s = malloc(sizeof(Sectionstats))) == NULL)
                fatal(memerr);
        return(s);

}

Stats *newstats()
{
        Stats *s;
        if((s = malloc(sizeof(Stats))) == NULL)
                fatal(memerr);
        return(s);
}

Ifile *newifile()
{
        Ifile *f;
        if((f = malloc(sizeof(Ifile))) == NULL) {
                fatal(memerr);
        }
        return(f);
}
