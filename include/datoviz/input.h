/*************************************************************************************************/
/*  Input                                                                                        */
/*************************************************************************************************/

#ifndef DVZ_HEADER_INPUT
#define DVZ_HEADER_INPUT



/*************************************************************************************************/
/*  Includes                                                                                     */
/*************************************************************************************************/

#include "_time.h"



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
    DvzWindow* window;
};



/*************************************************************************************************/
/*  Window input functions                                                                       */
/*************************************************************************************************/

DvzInput* dvz_input(DvzWindow* window);

DvzMouse* dvz_input_mouse(DvzInput* input);

DvzKeyboard* dvz_input_keyboard(DvzInput* input);

void dvz_input_destroy(DvzInput* input);



#endif
