#ifndef STRATIN_H
#define STRATIN_H 

/*

*/
struct StratPackage
{
        char* hrd; 
        char* sft; 
        char* splt; 
        double* cnt;
};

/*   
Functions to retrieve the appropriate actions from the templated strategy input
files. 

THESE ARE NOT MEMORY SAFE, AND CARE MUST BE TAKEN IN THE CALLING STATE TO AVOID 
MISINDEXING. 

The contiguous C array from Numpy memoryviews will rely on this file to be 
accessed at the C++ level correctly.
*/

// :p: player value
// :d: dealer value

inline char hrdActionFromPtr(char* stratHead, unsigned int p, unsigned int d) 
{
        return *(stratHead + (p-4)*10 + (d-2));
}

inline char sftActionFromPtr(char* stratHead, unsigned int p, unsigned int d)
{
        return *(stratHead + (p-13)*10 + (d-2));
}

inline char spltActionFromPtr(char* stratHead, unsigned int p, unsigned int d) 
{
        return *(stratHead + (p-2)*10 + (d-2));
}

inline double cntFromPtr(double* countHead, unsigned int val)
{
        return *(countHead + (val-2));
}

#endif