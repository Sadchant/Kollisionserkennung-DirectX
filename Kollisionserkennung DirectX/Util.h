#pragma once

#define SAFERELEASE(X) if (X) { X->Release(); X = 0; }
#define DELETEVAR(X) if (X) { delete X; X = 0; }
#define SAFEDELETEARRAY(X) if (X) { delete[] X; X = 0; }

#define RETURN_FALSE_IF_FAIL(X) if (FAILED(X)) return false;