/*************************************************************************************************/
/*  Input                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_INPUT
#define DVZ_HEADER_INPUT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "common.h"



/*************************************************************************************************/
/*  Typedefs                                                                                     */
/*************************************************************************************************/

typedef struct DvzInput DvzInput;

// Forward declarations.
typedef struct DvzKeyboard DvzKeyboard;
typedef struct DvzMouse DvzMouse;
typedef struct DvzWindow DvzWindow;


/*************************************************************************************************/
/*  Structs                                                                                      */
/*************************************************************************************************/

struct DvzInput
{
    DvzMouse* mouse;
    DvzKeyboard* keyboard;
    DvzClock clock;
};



/*************************************************************************************************/
/*  Window input functions                                                                       */
/*************************************************************************************************/

DVZ_EXPORT DvzInput* dvz_input(DvzWindow* window);

DVZ_EXPORT DvzMouse* dvz_input_mouse(DvzInput* input);

DVZ_EXPORT DvzKeyboard* dvz_input_keyboard(DvzInput* input);

DVZ_EXPORT void dvz_input_destroy(DvzInput* input);



#endif
