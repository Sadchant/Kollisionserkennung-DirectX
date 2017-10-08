#include "HlslSharedDefines.h"


RWStructuredBuffer<float3> intersectCenters : register(u0);

[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint id = DTid.x;
    uint length, stride;
    intersectCenters.GetDimensions(length, stride);

    if (id > length)
        return;

    intersectCenters[id] = float3(0, 0, 0);

}