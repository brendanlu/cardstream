#ifndef STRATIN_H
#define STRATIN_H 

/*   
Our contiguous C array from Numpy memoryviews will rely on this file to be
        accessed at the C++ level correctly.

We use templated strategy input file , so we can hardcode this logic into inline functions.
*/

// d is dealer upcard value
inline char hrdActionFromPtr(char* head, unsigned int p, unsigned int d) 
{ // there are multiple ways to get the same hard tally p 
        return *(head + (p-4)*10 + (d-2));
}

inline char sftActionFromPtr(char* head, unsigned int p, unsigned int d)
{ // there are multiple ways to get the same soft tally p
        return *(head + (p-13)*10 + (d-2));
}

inline char spltActionFromPtr(char* head, unsigned int p, unsigned int d) 
{ // p is the value the player has double of
        return *(head + (p-2)*10 + (d-2));
}

inline double cntFromPtr(double* head, char face)
{
        return *(head + (face-2));
}

#endif