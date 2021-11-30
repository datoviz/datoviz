/*************************************************************************************************/
/*  Tests                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_TEST_TRANSFERS
#define DVZ_HEADER_TEST_TRANSFERS



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "testing.h"
#include "vklite.h"



/*************************************************************************************************/
/*  Transfers tests                                                                              */
/*************************************************************************************************/

int test_transfers_buffer_mappable(TstSuite*);

int test_transfers_buffer_large(TstSuite*);

int test_transfers_buffer_copy(TstSuite*);

int test_transfers_image_buffer(TstSuite*);

int test_transfers_direct_buffer(TstSuite*);

int test_transfers_direct_image(TstSuite*);

int test_transfers_dups_util(TstSuite* tc);

int test_transfers_dups_upload(TstSuite* tc);

int test_transfers_dups_copy(TstSuite* tc);



#endif
