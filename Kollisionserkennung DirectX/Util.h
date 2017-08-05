#pragma once

#define RELEASEBUFFER(X) if (X) { X->Release(); X = 0; }
#define DELETEVAR(X) if (X) { delete X; X = 0; }
#define DELETEARRAY(X) if (X) { delete[] X; X = 0; }
