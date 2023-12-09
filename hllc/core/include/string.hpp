#include "memory.hpp"
#include <cstring>

#ifndef HLLC_STRING
#define HLLC_STRING

namespace HLLC {
    struct String {
        char *s;
        int l;
        String() {
            s = nullptr;
            l = 0;
        }
        String(std::nullptr_t) {
            s = nullptr;
            l = 0;
        }
        explicit String(int len) {
            l = len;
            s = (char*)memalloc((l+1)*sizeof(char));
        }
        String(const char* cs) {
            auto slen = strlen(cs);
            l = slen;
            s = (char*)memalloc((l+1)*sizeof(char));
            if (cs == nullptr) {
                s[0] = 0;
            } else {
                strcpy(s, cs);
            }
        }
        String(const char* cs, size_t len) {
            l = len;
            s = (char*)memalloc((l+1)*sizeof(char));
            if (cs == nullptr) {
                s[0] = 0;
            } else {
                memcpy(s,cs,(l+1)*sizeof(char));
            }
        }
        String(const String& o) {
            l = o.l;
            s = (char*)memalloc((l+1)*sizeof(char));
            if (o.s == nullptr) {
                s[0] = 0;
            } else {
                memcpy(s,o.s,(l+1)*sizeof(char));
            }
        }
        ~String() {
            if (s != nullptr) memfree(s);
        }
        String& operator=(const String& o) {
            l = o.l;
            s = (char*)memalloc((l+1)*sizeof(char));
            if (o.s == nullptr) {
                s[0] = 0;
            } else {
                memcpy(s,o.s,(l+1)*sizeof(char));
            }
            return *this;
        }
        operator const char* () const {
            return s;
        }
        bool operator==(std::nullptr_t) const {
            return s == nullptr;
        }
        bool operator==(const char* s) const {
            return strcmp(s, this->s) == 0;
        }
        bool operator==(const String& o) const {
            return o.l==l and memcmp(o.s, s, l) == 0;
        }
        bool operator==(const String&& o) const {
            return o.l==l and memcmp(o.s, s, l) == 0;
        }
        bool operator!=(std::nullptr_t) const {
            return s != nullptr;
        }
        bool operator!=(const char* s) const {
            return strcmp(s, this->s) != 0;
        }
        bool operator!=(const String& o) const {
            return o.l!=l or memcmp(o.s, s, l) != 0;
        }
        bool operator!=(const String&& o) const {
            return o.l!=l or memcmp(o.s, s, l) != 0;
        }
    };
}

#endif