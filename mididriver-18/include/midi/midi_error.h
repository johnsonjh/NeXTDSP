/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 07-Jun-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

/*
 * Print an error message for the specified return value.
 */
void midi_error(const char *s, kern_return_t r);

/*
 * Return the error string associated with the specified return value.
 */
const char *midi_error_string(kern_return_t r);

