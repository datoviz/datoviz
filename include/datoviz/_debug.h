/*************************************************************************************************/
/*  Debugging macros                                                                             */
/*************************************************************************************************/

#ifndef DVZ_HEADER_DEBUG
#define DVZ_HEADER_DEBUG



#define DBG(x)  printf("%" PRIu64 "\n", (x));
#define DBGS(x) printf("%" PRId64 "\n", (x));
#define DBGF(x) printf("%.8f\n", (double)(x))
#define PRT(x)  printf("%s\n", (x))



#endif
