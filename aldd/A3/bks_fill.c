int bks_fill(void* dest, unsigned int size, const char* str, int len)
{
    int rc = 0;
    int i = 0; // an index into the source string
    char* p = (char*)dest;
    char* end = dest + size;// one past last location in dest buffer.

    if (!(dest && str && 0 < len)) {
        rc = -1;
        goto exit;
    }

    while (p != end) {
        *p++ = str[i];
        i = (i + 1) % len;
    }

exit:
    return rc;
}
