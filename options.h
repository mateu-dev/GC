#pragma once

#include "Graph.h"
#include "Node.h"
#include <iostream>

#define SQLITE_DATABASE_PATH "./export/results.db"
#define DEBUG


// UTIL

#define PRINT(A)
#ifdef DEBUG
#define PRINT(A) std::cout<<A<<"\n";
#endif // DEBUG

using Graph_ = Graph<Node>;