#ifndef DAE_REI_MACROS
#define DAE_REI_MACROS
#define SAFE_RELEASE(ptr) {if(ptr){ptr->Release(); ptr = nullptr;}}
#define SAFE_DELETE(ptr) {if(ptr){delete ptr; ptr = nullptr;}}
#endif