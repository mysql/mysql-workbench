// prefix header for xcode

#include <list>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <stdexcept>

#ifdef __APPLE__
#undef nil
#define nil empty
#endif
#include <sigc++/sigc++.h>

#ifdef __GNUC__
#include <ext/hash_set>
#define ext __gnu_cxx
#else
#include <hash_set>
#define ext std
#endif

#include <math.h>

#include "cairo/cairo.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#elif defined(WIN32)
#else
#include <GL/gl.h>
#endif
