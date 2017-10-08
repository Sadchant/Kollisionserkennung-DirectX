#include "HlslSharedDefines.h"

RWStructuredBuffer<float3> intersectCenters : register(u0); // Buffer, der mit 0en gef�llt werden soll

// Bef�llt den intersectCenters-Ergebnisbuffer wieder mit 0en (per Shader schneller als UpdateSubresource von der CPU aus)
[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint id = DTid.x;
    uint length, stride;
    intersectCenters.GetDimensions(length, stride);

    if (id > length)
        return;

    // schreibe an jede Stelle eine 0
    intersectCenters[id] = float3(0, 0, 0);
}