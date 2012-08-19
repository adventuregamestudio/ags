
#ifdef _DEBUG

inline void assert(bool expr, const char *err_msg = "assertion failed")
{
    if (!expr) {
        _asm {
            int 3
        }
    }
}

#endif // _DEBUG
