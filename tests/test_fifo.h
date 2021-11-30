/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_FIFO
#define DVZ_HEADER_TEST_FIFO



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"



/*************************************************************************************************/
/*  FIFO tests                                                                                   */
/*************************************************************************************************/

int test_utils_fifo_1(TstSuite*);

int test_utils_fifo_2(TstSuite*);

int test_utils_fifo_resize(TstSuite*);

int test_utils_fifo_discard(TstSuite*);

int test_utils_fifo_first(TstSuite*);



/*************************************************************************************************/
/*  Deq tests                                                                                    */
/*************************************************************************************************/

int test_utils_deq_1(TstSuite*);

int test_utils_deq_2(TstSuite*);

int test_utils_deq_dependencies(TstSuite*);

int test_utils_deq_circular(TstSuite*);

int test_utils_deq_proc(TstSuite*);

int test_utils_deq_wait(TstSuite*);

int test_utils_deq_batch(TstSuite*);



#endif
