#ifndef RBLX_BASIC_TYPES
#define RBLX_BASIC_TYPES

#include <cstddef>
#include <godot_cpp/variant/variant.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/templates/hashfuncs.hpp>
#include <string.h>
#include "rblx_debug.hpp"
namespace godot {


struct LuaString {
    char *s;
    int l;
    LuaString(String s) {
        CharString c = s.ascii();
        l = c.size();
        this->s = (char*)memalloc((l+1)*sizeof(char));
        if (c.ptr() == nullptr) {
            this->s[0] = '\0';
        } else {
            memcpy(this->s,c.ptr(),(l+1)*sizeof(char));
        }
    }
    LuaString() {
        s = nullptr;
        l = 0;
    }
    LuaString(std::nullptr_t) {
        s = nullptr;
        l = 0;
    }
    explicit LuaString(int len) {
        l = len;
        s = (char*)memalloc((l+1)*sizeof(char));
    }
    LuaString(const char* cs) {
        auto slen = strlen(cs);
        l = slen;
        s = (char*)memalloc((l+1)*sizeof(char));
        if (cs == nullptr) {
            s[0] = 0;
        } else {
            strcpy(s, cs);
        }
    }
    LuaString(const char* cs, size_t len) {
        l = len;
        s = (char*)memalloc((l+1)*sizeof(char));
        if (cs == nullptr) {
            s[0] = 0;
        } else {
            memcpy(s,cs,(l+1)*sizeof(char));
        }
    }
    LuaString(const LuaString& o) {
        l = o.l;
        s = (char*)memalloc((l+1)*sizeof(char));
        if (o.s == nullptr) {
            s[0] = 0;
        } else {
            memcpy(s,o.s,(l+1)*sizeof(char));
        }
    }
    ~LuaString() {
        if (s != nullptr) memfree(s);
    }
    LuaString& operator=(const LuaString& o) {
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
    bool operator==(const LuaString& o) const {
        return o.l==l and memcmp(o.s, s, l) == 0;
    }
    bool operator==(const LuaString&& o) const {
        return o.l==l and memcmp(o.s, s, l) == 0;
    }
    bool operator!=(std::nullptr_t) const {
        return s != nullptr;
    }
    bool operator!=(const char* s) const {
        return strcmp(s, this->s) != 0;
    }
    bool operator!=(const LuaString& o) const {
        return o.l!=l or memcmp(o.s, s, l) != 0;
    }
    bool operator!=(const LuaString&& o) const {
        return o.l!=l or memcmp(o.s, s, l) != 0;
    }
};

class LuaStringHasher {
public:
    _FORCE_INLINE_ static uint32_t hash(const LuaString& s) {
        return hash_djb2_buffer((const uint8_t*)s.s,sizeof(char)*s.l);
    }
};

struct Axes {
    bool X,Y,Z,Top,Bottom,Left,Right,Back,Front = false;
};
struct Color8 {
    unsigned char r,g,b = 0;
};
struct Color8A : public Color8 {
    unsigned char a = 0;
};
struct BrickColor {
    unsigned short id = 1;
    unsigned char r,g,b,a = 0;
    BrickColor(unsigned short id) : id(id) {
        if (color_ids.size() == 0) {
            color_ids[1] =  {242,243,243,255};
            color_ids[2] =  {161,165,162,255};
            color_ids[3] =  {249,233,153,255};
            color_ids[5] =  {215,197,154,255};
            color_ids[6] =  {194,218,184,255};
            color_ids[9] =  {232,186,200,255};
            color_ids[11] = {128,187,219,255};
            color_ids[12] = {203,132, 66,255};
            color_ids[18] = {204,142,105,255};
            color_ids[21] = {196, 40, 28,255};
            color_ids[22] = {196,112,160,255};
            color_ids[23] = { 13,105,172,255};
            color_ids[24] = {245,205, 48,255};
            color_ids[25] = { 98, 71, 50,255};
            color_ids[26] = { 27, 42, 53,255};
            color_ids[27] = {109,110,108,255};
            color_ids[28] = { 40,127, 71,255};
            // TODO: fill this up and make it work
            //.... im too lazy im so sorry
        }
    }
    static HashMap<unsigned short, Color8A> color_ids;
};
struct RBXVector2 {
    double X,Y;
    // TODO: other stuff
};
struct RBXVector3 {
    double X,Y,Z;
    
    RBXVector3 operator+(const RBXVector3& o) {
        RBXVector3 v3;
        v3.X = X+o.X;
        v3.Y = Y+o.Y;
        v3.Z = Z+o.Z;
        return v3;
    }
    RBXVector3 operator-(const RBXVector3& o) {
        RBXVector3 v3;
        v3.X = X-o.X;
        v3.Y = Y-o.Y;
        v3.Z = Z-o.Z;
        return v3;
    }
    RBXVector3 operator*(const RBXVector3& o) {
        RBXVector3 v3;
        v3.X = X*o.X;
        v3.Y = Y*o.Y;
        v3.Z = Z*o.Z;
        return v3;
    }
    RBXVector3 operator/(const RBXVector3& o) {
        RBXVector3 v3;
        v3.X = X/o.X;
        v3.Y = Y/o.Y;
        v3.Z = Z/o.Z;
        return v3;
    }
    RBXVector3 operator*(const double n) {
        RBXVector3 v3;
        v3.X = X*n;
        v3.Y = Y*n;
        v3.Z = Z*n;
        return v3;
    }
    RBXVector3 operator/(const double n) {
        RBXVector3 v3;
        v3.X = X/n;
        v3.Y = Y/n;
        v3.Z = Z/n;
        return v3;
    }

    static RBXVector3 new_(const double x, const double y, const double z) {
        return {x, y, z};
    }
    //TODO: static RBXVector3 fromNormalId
    //TODO: static RBXVector3 fromAxis

    double getMagnitude() const {
        return sqrt(X*X+Y*Y+Z*Z);
    }
    RBXVector3 getUnit() const {
        const double m = getMagnitude();
        return {X/m,Y/m/Z/m};
    }
    static constexpr RBXVector3 ONE = {1,1,1};
    static constexpr RBXVector3 ZERO = {0,0,0};
    // TODO: other stuff
};
struct CFrame {
    double R00,R10,R20,R01,R11,R21,R02,R12,R22,X,Y,Z = 0;
    CFrame()=default;
    CFrame(RBXVector3 pos) {
        X=pos.X; Y=pos.Y; Z=pos.Z; 
    }
    CFrame(double x, double y, double z) {
        X=x; Y=y; Z=z; 
    }
    // TODO: other stuff
};
struct Color3 {
private:
    inline constexpr static double clamp(double n) {
        return (n > 1) ? 1 : (n < 0) ? 0 : n;
    }
    inline constexpr static double lerp(double from, double to, double a) {
        return from*(1-a)+to*a;
    }
    static constexpr char hex[17] = "0123456789ABCDEF";
    inline constexpr char asHex0(unsigned char n) {
        return hex[n%16];
    }
    inline constexpr char asHex1(unsigned char n) {
        return hex[(n<<4)%16];
    }
    inline constexpr void asHex(char *s, unsigned char n) {
        s[0]=asHex1(n);s[1]=asHex0(n);
    }
public:
    double R,G,B=0.0;
    Color3() {}
    Color3(double r) { R=clamp(r); }
    Color3(double r, double g) { R=r; G=clamp(g); }
    Color3(double r, double g, double b) { R=clamp(r); G=clamp(g); B=clamp(b); }
    static Color3 fromRGB(double red, double green, double blue) {
        return Color3(clamp(red/255),clamp(green/255),clamp(blue/255));
    }
    Color3 Lerp(Color3& to, double alpha) {
        alpha = clamp(alpha);
        return Color3(lerp(R,to.R,alpha),lerp(G,to.G,alpha),lerp(B,to.B,alpha));
    }
    LuaString ToHex() {
        LuaString s = LuaString(7);
        s.s[0] = '#';
        asHex(s.s+1, R);
        asHex(s.s+3, G);
        asHex(s.s+5, B);
        return s;
    }
};
struct ColorSequenceKeypoint {
    double time;
    Color3 value;
};
struct ColorSequence {
    Vector<ColorSequenceKeypoint> Keypoints;
    ColorSequence(Color3 c) {
        Keypoints.append({0,c});
    }
    ColorSequence(Color3 c0, Color3 c1) {
        Keypoints.append({0,c0}); Keypoints.append({1,c1});
    }
    ColorSequence(Vector<ColorSequenceKeypoint> keypoints) {
        Keypoints = keypoints;
    }
};


};
#endif