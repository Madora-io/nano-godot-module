#ifndef DODECAHEDRON_BIGINT_H_
#define DODECAHEDRON_BIGINT_H_

#include <vector>
#include <iostream>
#include <map>

#include "core/script_language.h"

namespace Dodecahedron
{
class Bigint
{
private:
    std::vector<int> number;
    bool positive;
    int base;
    unsigned int skip;
    static const int default_base=INT_MAX;

public:
    //Constructors
    Bigint();
    Bigint(String);
    Bigint(const Bigint& b);

    //Allocation
    Bigint operator=(String);
    Bigint operator=(Bigint);

    //Adding
    Bigint operator+(Bigint const &) const;
    Bigint &operator+=(Bigint const &);
    Bigint operator+(long long const &) const;
    Bigint &operator+=(long long);

    //Subtraction
    Bigint operator-(Bigint const &) const;
    Bigint &operator-=(Bigint const &);

    //Multiplication
    Bigint operator*(Bigint const &);
    Bigint &operator*=(Bigint const &);
    Bigint operator*(long long const &);
    Bigint &operator*=(int const &);

    //Compare
    bool operator<(const Bigint &) const;
    bool operator>(const Bigint &) const;
    bool operator<=(const Bigint &) const;
    bool operator>=(const Bigint &) const;
    bool operator==(const Bigint &) const;
    bool operator!=(const Bigint &) const;

    //Access
    int operator[](int const &);

    //Input&Output
    String to_string();
    String to_hex();

    //Helpers
    void clear();
    Bigint &abs();

    //Power
    Bigint &pow(int const &);

    //Trivia
    int digits() const;
    int trailing_zeros() const;
private:
    int segment_length(int) const;
    Bigint pow(int const &, std::map<int, Bigint> &);
    int compare(Bigint const &) const; //0 a == b, -1 a < b, 1 a > b
};

Bigint abs(Bigint);
Bigint factorial(int);
}

#endif
