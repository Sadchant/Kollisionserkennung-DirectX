#include "HlslSharedDefines.h"
#include "0_ComputeShaderGlobals.hlsl"

StructuredBuffer<float3> minInput : register(t0);
StructuredBuffer<float3> maxInput : register(t1);

RWStructuredBuffer<float3> minOutput : register(u0);
RWStructuredBuffer<float3> maxOutput : register(u1);

// groupshared, also greife nur mit groupLocalID darauf zu, sonst k�nnte man aus dem Speicher laufen!
groupshared float3 minTemp[LINEAR_XTHREADS];
groupshared float3 maxTemp[LINEAR_XTHREADS];

cbuffer reduceData : register(b0)
{
    uint firstStepStride;
    uint inputSize;
    uint bool_OutputIsInput;
}


[numthreads(LINEAR_XTHREADS, LINEAR_YTHREADS, LINEAR_ZTHREADS)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
    uint id = DTid.x;
    uint groupLocalID = GTid.x;
    uint groupID = Gid.x;

    // f�r die IDs, die im ersten Schritt keinen Counterpart auf der inaktiven Seite haben
    if ((id + firstStepStride) >= inputSize)
    {
        if (!bool_OutputIsInput)
        {
            minTemp[groupLocalID] = minInput[id];
            maxTemp[groupLocalID] = maxInput[id];
        }
        else // wenn Output der Input ist, lese vom Output :)
        {
            minTemp[groupLocalID] = minOutput[id];
            maxTemp[groupLocalID] = maxOutput[id];
        }
    }

    // im ersten Schritt die eine H�lfte der Daten aus Input mit der anderen H�lfte vergleichen
    // firStstepStride ist ~inputsize/2
    // das Ergebnis steht danach in min/maxTemp, also ab jetzt min/maxTemp weiterverarbeiten
    if (!bool_OutputIsInput)
    {
        minTemp[groupLocalID].x = min(minInput[id].x, minInput[id + firstStepStride].x);
        minTemp[groupLocalID].y = min(minInput[id].y, minInput[id + firstStepStride].y);
        minTemp[groupLocalID].z = min(minInput[id].z, minInput[id + firstStepStride].z);

        maxTemp[groupLocalID].x = max(maxInput[id].x, maxInput[id + firstStepStride].x);
        maxTemp[groupLocalID].y = max(maxInput[id].y, maxInput[id + firstStepStride].y);
        maxTemp[groupLocalID].z = max(maxInput[id].z, maxInput[id + firstStepStride].z);
    }
    else // wenn Output der Input ist, lese vom Output :)
    {
        minTemp[groupLocalID].x = min(minOutput[id].x, minOutput[id + firstStepStride].x);
        minTemp[groupLocalID].y = min(minOutput[id].y, minOutput[id + firstStepStride].y);
        minTemp[groupLocalID].z = min(minOutput[id].z, minOutput[id + firstStepStride].z);

        maxTemp[groupLocalID].x = max(maxOutput[id].x, maxOutput[id + firstStepStride].x);
        maxTemp[groupLocalID].y = max(maxOutput[id].y, maxOutput[id + firstStepStride].y);
        maxTemp[groupLocalID].z = max(maxOutput[id].z, maxOutput[id + firstStepStride].z);
    }
    // stelle sicher dass alle Threads der Gruppe mit dem ersten Schritt fertig sind
    GroupMemoryBarrierWithGroupSync();

    // der Rest der Daten wird iterativ weiterverarbeitet, bis die aktuelle Gruppe ihren Bereich reduziert hat
    for (uint i = LINEAR_XTHREADS / 2; i > 0; i /= 2)
    {
        // die Threads, die im Teil der inaktiven Daten liegen, bekommen nie wieder etwas zu tun in diesem Dispatch und k�nnen returnen
        if (groupLocalID > i)
        {
            // der Compiler kann kein return in einer for-schleife, wenn sie AllMemoryBarrierWithGroupSync beinhaltet, 
            // sonst kommt "thread sync operation found in varying flow control", also leeres if-else
        }
        // beim letzten Durchlauf insgesamt, wenn es nur noch einen Block gibt, kann es sein dass man wieder �ber die Daten hinausgeht mit id + i, deswegen:
        else if (id + i >= inputSize)
        {
            if (!bool_OutputIsInput)
            {
                minTemp[groupLocalID] = minInput[id];
                maxTemp[groupLocalID] = maxInput[id];
            }
            else // wenn Output der Input ist, lese vom Output :)
            {
                minTemp[groupLocalID] = minOutput[id];
                maxTemp[groupLocalID] = maxOutput[id];
            }
        }
        else
        {
            // f�hre die Reduktion durch, die StepStride ist in der for-Schleife i
            minTemp[groupLocalID].x = min(minTemp[groupLocalID].x, minTemp[groupLocalID + i].x);
            minTemp[groupLocalID].y = min(minTemp[groupLocalID].y, minTemp[groupLocalID + i].y);
            minTemp[groupLocalID].z = min(minTemp[groupLocalID].z, minTemp[groupLocalID + i].z);

            maxTemp[groupLocalID].x = max(maxTemp[groupLocalID].x, maxTemp[groupLocalID + i].x);
            maxTemp[groupLocalID].y = max(maxTemp[groupLocalID].y, maxTemp[groupLocalID + i].y);
            maxTemp[groupLocalID].z = max(maxTemp[groupLocalID].z, maxTemp[groupLocalID + i].z);
        }
        // Blockiere, bis alle Threads dieser Gruppe die Speicherzugriffe dieses Schleifendurchlaufs durchgef�hrt haben
        // wir m�ssen nicht auf AllMemory, da s�mtliche Ergebnisse ja in groupshared Speicher min/maxTemp geschrieben werden
        GroupMemoryBarrierWithGroupSync();
    }
    // der letzte Thread, der �brig bleibt hat die ID, die auf dem Ergebnis dieser Gruppe liegt
    // kopiere das Ergebnis von Temp in Output, beachte, dass Output einen Eintrag pro Gruppe hat!
    if (groupLocalID == 0) // kann zwar eh immer nur Thread 0 pro Gruppe sein, der hier ankommt, aber sonst motzt der Compiler wegen Race Conditions
    {
        minOutput[groupID] = minTemp[groupLocalID];
        maxOutput[groupID] = maxTemp[groupLocalID];
    }
}