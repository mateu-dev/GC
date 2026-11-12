#pragma once

#include <iostream>

#define DEBUG

#define INDEX_FROM_ONE


// UTIL

#define PRINT(A)
#ifdef DEBUG
#define PRINT(A) std::cout<<A<<"\n";
#endif // DEBUG