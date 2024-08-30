/*
* Copyright (c) 2021 Cyrille Rossant and contributors. All rights reserved.
* Licensed under the MIT license. See LICENSE file in the project root for details.
* SPDX-License-Identifier: MIT
*/

/*************************************************************************************************/
/*  Key code                                                                                     */
/*************************************************************************************************/

#ifndef DVZ_HEADER_KEYCODE
#define DVZ_HEADER_KEYCODE



/*************************************************************************************************/
/*  Keyboard key codes (must correspond to glfw for now)                                         */
/*************************************************************************************************/

typedef enum
{
    DVZ_KEY_UNKNOWN = -1,
    DVZ_KEY_NONE = +0,
    DVZ_KEY_SPACE = 32,
    DVZ_KEY_APOSTROPHE = 39, /* ' */
    DVZ_KEY_COMMA = 44,      /* , */
    DVZ_KEY_MINUS = 45,      /* - */
    DVZ_KEY_PERIOD = 46,     /* . */
    DVZ_KEY_SLASH = 47,      /* / */
    DVZ_KEY_0 = 48,
    DVZ_KEY_1 = 49,
    DVZ_KEY_2 = 50,
    DVZ_KEY_3 = 51,
    DVZ_KEY_4 = 52,
    DVZ_KEY_5 = 53,
    DVZ_KEY_6 = 54,
    DVZ_KEY_7 = 55,
    DVZ_KEY_8 = 56,
    DVZ_KEY_9 = 57,
    DVZ_KEY_SEMICOLON = 59, /* ; */
    DVZ_KEY_EQUAL = 61,     /* = */
    DVZ_KEY_A = 65,
    DVZ_KEY_B = 66,
    DVZ_KEY_C = 67,
    DVZ_KEY_D = 68,
    DVZ_KEY_E = 69,
    DVZ_KEY_F = 70,
    DVZ_KEY_G = 71,
    DVZ_KEY_H = 72,
    DVZ_KEY_I = 73,
    DVZ_KEY_J = 74,
    DVZ_KEY_K = 75,
    DVZ_KEY_L = 76,
    DVZ_KEY_M = 77,
    DVZ_KEY_N = 78,
    DVZ_KEY_O = 79,
    DVZ_KEY_P = 80,
    DVZ_KEY_Q = 81,
    DVZ_KEY_R = 82,
    DVZ_KEY_S = 83,
    DVZ_KEY_T = 84,
    DVZ_KEY_U = 85,
    DVZ_KEY_V = 86,
    DVZ_KEY_W = 87,
    DVZ_KEY_X = 88,
    DVZ_KEY_Y = 89,
    DVZ_KEY_Z = 90,
    DVZ_KEY_LEFT_BRACKET = 91,  /* [ */
    DVZ_KEY_BACKSLASH = 92,     /* \ */
    DVZ_KEY_RIGHT_BRACKET = 93, /* ] */
    DVZ_KEY_GRAVE_ACCENT = 96,  /* ` */
    DVZ_KEY_WORLD_1 = 161,      /* non-US #1 */
    DVZ_KEY_WORLD_2 = 162,      /* non-US #2 */
    DVZ_KEY_ESCAPE = 256,
    DVZ_KEY_ENTER = 257,
    DVZ_KEY_TAB = 258,
    DVZ_KEY_BACKSPACE = 259,
    DVZ_KEY_INSERT = 260,
    DVZ_KEY_DELETE = 261,
    DVZ_KEY_RIGHT = 262,
    DVZ_KEY_LEFT = 263,
    DVZ_KEY_DOWN = 264,
    DVZ_KEY_UP = 265,
    DVZ_KEY_PAGE_UP = 266,
    DVZ_KEY_PAGE_DOWN = 267,
    DVZ_KEY_HOME = 268,
    DVZ_KEY_END = 269,
    DVZ_KEY_CAPS_LOCK = 280,
    DVZ_KEY_SCROLL_LOCK = 281,
    DVZ_KEY_NUM_LOCK = 282,
    DVZ_KEY_PRINT_SCREEN = 283,
    DVZ_KEY_PAUSE = 284,
    DVZ_KEY_F1 = 290,
    DVZ_KEY_F2 = 291,
    DVZ_KEY_F3 = 292,
    DVZ_KEY_F4 = 293,
    DVZ_KEY_F5 = 294,
    DVZ_KEY_F6 = 295,
    DVZ_KEY_F7 = 296,
    DVZ_KEY_F8 = 297,
    DVZ_KEY_F9 = 298,
    DVZ_KEY_F10 = 299,
    DVZ_KEY_F11 = 300,
    DVZ_KEY_F12 = 301,
    DVZ_KEY_F13 = 302,
    DVZ_KEY_F14 = 303,
    DVZ_KEY_F15 = 304,
    DVZ_KEY_F16 = 305,
    DVZ_KEY_F17 = 306,
    DVZ_KEY_F18 = 307,
    DVZ_KEY_F19 = 308,
    DVZ_KEY_F20 = 309,
    DVZ_KEY_F21 = 310,
    DVZ_KEY_F22 = 311,
    DVZ_KEY_F23 = 312,
    DVZ_KEY_F24 = 313,
    DVZ_KEY_F25 = 314,
    DVZ_KEY_KP_0 = 320,
    DVZ_KEY_KP_1 = 321,
    DVZ_KEY_KP_2 = 322,
    DVZ_KEY_KP_3 = 323,
    DVZ_KEY_KP_4 = 324,
    DVZ_KEY_KP_5 = 325,
    DVZ_KEY_KP_6 = 326,
    DVZ_KEY_KP_7 = 327,
    DVZ_KEY_KP_8 = 328,
    DVZ_KEY_KP_9 = 329,
    DVZ_KEY_KP_DECIMAL = 330,
    DVZ_KEY_KP_DIVIDE = 331,
    DVZ_KEY_KP_MULTIPLY = 332,
    DVZ_KEY_KP_SUBTRACT = 333,
    DVZ_KEY_KP_ADD = 334,
    DVZ_KEY_KP_ENTER = 335,
    DVZ_KEY_KP_EQUAL = 336,
    DVZ_KEY_LEFT_SHIFT = 340,
    DVZ_KEY_LEFT_CONTROL = 341,
    DVZ_KEY_LEFT_ALT = 342,
    DVZ_KEY_LEFT_SUPER = 343,
    DVZ_KEY_RIGHT_SHIFT = 344,
    DVZ_KEY_RIGHT_CONTROL = 345,
    DVZ_KEY_RIGHT_ALT = 346,
    DVZ_KEY_RIGHT_SUPER = 347,
    DVZ_KEY_MENU = 348,
    DVZ_KEY_LAST = 348,
} DvzKeyCode;



#endif
