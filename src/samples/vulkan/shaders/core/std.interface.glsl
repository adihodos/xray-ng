#if defined(__VERT_SHADER__)
#define INTERFACE_BLOCK out
#define BLOCKVAR vs_out
#elif defined(__FRAG_SHADER__)
#define INTERFACE_BLOCK in
#define BLOCKVAR fs_in
#else
#error "Unkown shader kind"
#endif

