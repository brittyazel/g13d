//
// Created by Britt Yazel on 03-16-2025.
//

#ifndef UTILITIES_TPP
#define UTILITIES_TPP

#include <map>

#include "exceptions.hpp"

namespace G13 {
    template <class T>
    const T& formatter(const T& v);

    template <class KEYT, class VALT>
    const VALT& find_or_throw(const std::map<KEYT, VALT>& m, const KEYT& target);

    template <class KEYT, class VALT>
    VALT& find_or_throw(std::map<KEYT, VALT>& m, const KEYT& target);

    template <class T>
    class Coord {
    public:
        Coord();
        Coord(T _x, T _y);
        T x;
        T y;
    };

    template <class T>
    std::ostream& operator<<(std::ostream& o, const Coord<T>& c);

    template <class T>
    class Bounds {
    public:
        using CT = Coord<T>;

        Bounds(const CT& _tl, const CT& _br);
        Bounds(T x1, T y1, T x2, T y2);

        bool contains(const CT& pos) const;
        void expand(const CT& pos);

        CT tl;
        CT br;
    };

    template <class T>
    std::ostream& operator<<(std::ostream& o, const Bounds<T>& b);

    template <class MAP_T>
    struct _map_keys_out {
        _map_keys_out(const MAP_T& c, std::string s);
        const MAP_T& container;
        std::string sep;
    };

    template <class STREAM_T, class MAP_T>
    STREAM_T& operator<<(STREAM_T& o, const _map_keys_out<MAP_T>& _mko);

    template <class MAP_T>
    _map_keys_out<MAP_T> map_keys_out(const MAP_T& c, const std::string& sep = " ");

    template <typename Container>
    Container split(const typename Container::value_type& srcStr, const typename Container::value_type& delimiters,
                    Empties empties = Empties::empties_ok);

    template <class T>
    const T& formatter(const T& v) {
        return v;
    }

    template <class KEYT, class VALT>
    const VALT& find_or_throw(const std::map<KEYT, VALT>& m, const KEYT& target) {
        auto i = m.find(target);
        if (i == m.end()) {
            throw NotFoundException();
        }
        return i->second;
    }

    template <class KEYT, class VALT>
    VALT& find_or_throw(std::map<KEYT, VALT>& m, const KEYT& target) {
        auto i = m.find(target);
        if (i == m.end()) {
            throw NotFoundException();
        }
        return i->second;
    }

    template <class T>
    Coord<T>::Coord() : x(), y() {}

    template <class T>
    Coord<T>::Coord(T _x, T _y) : x(_x), y(_y) {}

    template <class T>
    std::ostream& operator<<(std::ostream& o, const Coord<T>& c) {
        o << "{ " << c.x << " x " << c.y << " }";
        return o;
    }

    template <class T>
    Bounds<T>::Bounds(const CT& _tl, const CT& _br) : tl(_tl), br(_br) {}

    template <class T>
    Bounds<T>::Bounds(T x1, T y1, T x2, T y2) : tl(x1, y1), br(x2, y2) {}

    template <class T>
    bool Bounds<T>::contains(const CT& pos) const {
        return tl.x <= pos.x && tl.y <= pos.y && pos.x <= br.x && pos.y <= br.y;
    }

    template <class T>
    void Bounds<T>::expand(const CT& pos) {
        if (pos.x < tl.x) tl.x = pos.x;
        if (pos.y < tl.y) tl.y = pos.y;
        if (pos.x > br.x) br.x = pos.x;
        if (pos.y > br.y) br.y = pos.y;
    }

    template <class T>
    std::ostream& operator<<(std::ostream& o, const Bounds<T>& b) {
        o << "{ " << b.tl.x << " x " << b.tl.y << " / " << b.br.x << " x " << b.br.y << " }";
        return o;
    }

    template <class MAP_T>
    _map_keys_out<MAP_T>::_map_keys_out(const MAP_T& c, std::string s) : container(c), sep(std::move(s)) {}

    template <class STREAM_T, class MAP_T>
    STREAM_T& operator<<(STREAM_T& o, const _map_keys_out<MAP_T>& _mko) {
        bool first = true;
        for (const auto& i : _mko.container) {
            if (first) {
                first = false;
                o << i.first;
            }
            else {
                o << _mko.sep << i.first;
            }
        }
        return o;
    }

    template <class MAP_T>
    _map_keys_out<MAP_T> map_keys_out(const MAP_T& c, const std::string& sep) {
        return _map_keys_out<MAP_T>(c, sep);
    }

    template <typename Container>
    Container split(const typename Container::value_type& srcStr, const typename Container::value_type& delimiters,
                    const Empties empties) {
        Container result;
        auto next = static_cast<size_t>(-1);
        do {
            if (empties == Empties::no_empties) {
                next = srcStr.find_first_not_of(delimiters, next + 1);
                if (next == Container::value_type::npos) {
                    break;
                }
                next -= 1;
            }
            size_t current = next + 1;
            next = srcStr.find_first_of(delimiters, current);
            result.push_back(srcStr.substr(current, next - current));
        }
        while (next != Container::value_type::npos);
        return result;
    }
}
#endif

