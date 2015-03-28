#include "stdafx.h"
#include "parallax.h"

namespace Parallax {

bool compareDepth(DepthImage *p1, DepthImage *p2)
{
    return p1->depth > p2->depth;
}

}