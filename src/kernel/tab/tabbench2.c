/*
 *     Benchmark some common table I/O operations
 *
 *     -  2" to read all lines using getline()  [100M benchmark see below]
 *     - 22" to break them using nemoinpr()  [this operation is several times slower in numpy]
 *                    (x,y,z) = np.loadtxt("tab3").T
 *     -  6" to use dofie()
 *     - < 1" using sqrt() - hard to measure it's too fast
 *     - for same size file, longer lines (thus fewer to read) is more efficient
 *
 *     24-jul-2020  V0.1    drafted
 *     25-jul-2020  V0.2    cleaned up
 *                  V0.4    test strtok() based parsing
 */


#include <stdinc.h>
#include <getparam.h>

string defv[] = {
    "in=???\n	       input file",
    "out=\n            output file",
    "mode=0\n          Benchmark mode",
    "nmax=10000\n      Default max allocation (not used)",
    "VERSION=0.5\n     25-jul-2020 PJT",
    NULL,
};

string usage="table I/O benchmark";

#ifndef MAX_LINELEN 
#define MAX_LINELEN  2048
#endif

#define MAXPAR 16

// these should go in some header file
extern int inifie(string);
extern void dofie(real *, int *, real *, real *);

// testing another method of parsing:
// NEMO's burststring() based version is 50% faster than this strtok() based version
// Q: is there a fancy sscanf method possible?
//    input :   *line
//    input :   *par
//    input :   npar elements of par[npar]
//    output:   ntok (number of parsed values)
//    On ouput  par[ntok] values are filled

typedef struct lls {
  //char *token;
  real val;
  struct lls *next;
} lls;
static real sum = 0;
static real *value;   //   value[0] , value[1], .... value[ntok-1]
static int size_of_value = 0;

//local int nemoinprt(char *line, real **par, int *npar)
  
// in tokenize npar is irrelevant, and it can return ntok > npar
local int nemoinprt(char *line, real *par, int npar)
{
  //           &       &
  //    'AAA   BBB     CCCC'
  //     *  0  *  0
  char *token = strtok(line," ,");
  int ntok = 0, counter, start_index;
  lls *curr, *prev = NULL, *start;

  /* tokenize and build linked list */

  while (token != NULL) {
    //par[ntok++] = atof(token);
    //token = strtok(NULL," ,");

    /* if (ntok == 0) {
      start = (lls *) allocate((ntok + 1) * sizeof(lls));
    } else {
      start = (lls *) reallocate(start, (ntok + 1) * sizeof(lls));
    } 

    curr = &(start[ntok]); */

    curr = (lls *) allocate(sizeof(lls));

    if (ntok == 0) {
      start = curr;
    }

    curr->val = atof(token);
    sum += curr->val;

    curr->next = NULL;

    prev = curr;
    curr = curr->next;
    ++ntok;

    token = strtok(NULL, " ,");
  }


  /* walk through the list of ntok elements , allocate *value
     and place the token pointers here
  */

  if (value == NULL) {
    value = (real *) allocate(ntok * sizeof(real));
    start_index = 0;
  } else {
    value = (real *) reallocate(value, size_of_value + ntok * sizeof(real));
    start_index = size_of_value;
  }

  // parse the linked list and copy over the values into an array for quick indexing
  curr = start;
  for (counter = start_index; counter < start_index + ntok; ++counter) {
    value[counter] = curr->val;
    curr = curr->next;
  }

  // size_of_value will keep track of the number of elements to skip past when adding new elements
  size_of_value += ntok;

  // free the linked list memory
  free(start);

  /* if ntok <= npar; fill those elements of the par[] */
  /* put warning if not , and only fill first npar */
  
  return ntok;
}

// pick the standard or local testing version in nemoinprt using gettok()
//#define my_nemoinpr nemoinpr
#define my_nemoinpr nemoinprt

void nemo_main(void)
{
    bool Qout = hasvalue("out");
    stream istr, ostr;
    int nmax,  *select = NULL;
    int nout, next = 0;
    int    npar, one = 1;
    size_t nlines;
    string iname = getparam("in");
    real par[MAXPAR], retval, errval;
    int mode = getiparam("mode");
    size_t linelen = MAX_LINELEN;
    //char line[MAX_LINELEN];
    char *line = malloc((linelen) * sizeof(char));        

    npar = inifie("sqrt(%1*%1+%2*%2+%3*%3)");
    dprintf(0,"MAX_LINELEN=%d\n",MAX_LINELEN);

    istr = stropen(getparam("in"),"r");
    if (Qout) ostr = stropen(getparam("out"),"w");

    nlines = 0;
    sum = 0.0;
    linelen = 0;
    if (mode <= 0) {
      if (mode < 0)
	dprintf(0,"Just reading\n");
      else
	dprintf(0,"Just reading and writing\n");
      //while (fgets(line,MAX_LINELEN,istr) != NULL)
      while (getline(&line, &linelen, istr) != -1) {
	nlines++;
	if (Qout && mode == 0) fputs(line,ostr);
      }
    } else if (mode == 1) {
        dprintf(0,"nemoinp to split line\n");      
        while (getline(&line, &linelen, istr) != -1) {
	        nlines++;
	        npar = my_nemoinpr(line,par,MAXPAR);
	      }
    } else if (mode == 2) {
        dprintf(0,"nemoinp + sqrt()\n");            
        while (getline(&line, &linelen, istr) != -1) {	  
	  nlines++;
	  npar = my_nemoinpr(line,par,MAXPAR);	
	  retval = sqrt(par[0]*par[0] + par[1]*par[1] + par[2]*par[2]);
	  sum += retval;
	  if (Qout) fprintf(ostr,"%s %g\n",line,retval);
	}
    } else if (mode == 3) {
        dprintf(0,"nemoinp + fie(sqrt())\n");                  
        while (getline(&line, &linelen, istr) != -1) {	        
	  nlines++;
	  npar = my_nemoinpr(line,par,MAXPAR);	
	  dofie(par,&one,&retval,&errval);
	  sum += retval;
	  if (Qout) fprintf(ostr,"%s %g\n",line,retval);	  
	}
    }

    strclose(istr);
    if (Qout) strclose(ostr);
    
    dprintf(0,"sum=%g\n",sum);
    dprintf(0,"Read %ld lines\n",nlines);

    free(value); // always free all allocated memory
}

// bench:
// tabgen tab1 1000000   3
// tabgen tab2 10000000  3
// tabgen tab3 100000000 3
// tabgen tab4 3 100000000

// fgets method on tab3: (cpu times approx. due to laptop variations)
//       BS    ST
// -1   1.9   1.8
//  0   3.3   3.2
//  1  25.9  40.6
//  2  26.6  38.7
//  3  31.2  43.2

//      /usr/bin/time tabbench2 tab2 . -1
//      /usr/bin/time tabbench2 tab2 . 0
//      /usr/bin/time tabbench2 tab2 . 1
//      /usr/bin/time tabbench2 tab2 . 2
//      /usr/bin/time tabbench2 tab2 . 3

// e.g. tabgen tab3    80 sec ->  34 MB/sec ( low level I/O :   435 MB/sec )

// T480  2.8 4.8 34.5 35.4 42.9
// X1Y4  1.9 3.1 28.6 28.6 34.4 (about 20% more for fie vs. compiled)
