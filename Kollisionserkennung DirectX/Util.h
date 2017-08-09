#pragma once

#define SAFERELEASE(X) if (X) { X->Release(); X = 0; }
#define DELETEVAR(X) if (X) { delete X; X = 0; }
#define SAFEDELETEARRAY(X) if (X) { delete[] X; X = 0; }

#define STARTTIMEMEASURE auto begin = high_resolution_clock::now();
#define ENDTIMEMEASURE(X) auto end = high_resolution_clock::now(); cout << X << ": " << duration_cast<milliseconds>(end - begin).count() << "ns" << endl;
