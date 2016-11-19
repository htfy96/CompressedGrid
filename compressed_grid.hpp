#ifndef COMPRESSED_GRID_HPP
#define COMPRESSED_GRID_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <climits>
#include <utility>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <utility>
#include <iostream>

#include "popcount.hpp"

namespace compgrid
{
    // GridPoint, {Black, White, NA}[W][H]
    template<std::size_t W, std::size_t H>
        struct GridPoint
        {
            char x, y;
            GridPoint() = default;
            GridPoint(char x, char y): x(x), y(y) {}
            void right()
            {
                ++y;
            }
            void left()
            {
                --y;
            }
            void down()
            {
                ++x;
            }
            void up()
            {
                --x;
            }
            GridPoint& operator++()
            {
                if (++y == W)
                {
                    y = 0;
                    ++x;
                }
                return *this;
            }
            GridPoint& operator--()
            {
                if (!y)
                {
                    y = W - 1;
                    --x;
                } else
                    --y;
                return *this;
            }
            bool is_top() const
            {
                return !x;
            }
            bool is_bottom() const
            {
                return x == H - 1;
            }
            bool is_left() const
            {
                return !y;
            }
            bool is_right() const
            {
                return y == W - 1;
            }
        };

    template<typename T, std::size_t W, std::size_t H, std::size_t obj_len_bit, typename UnderLyingType_ = unsigned long long>
        class CompressedGrid
        {
            static_assert(obj_len_bit > 0 && obj_len_bit <= (sizeof(UnderLyingType_) - 1) * CHAR_BIT,
                        // if obj_len_bit == 64(8 bytes), its read may across 9 aligned bytes!
                        "Object too large to be stored (> size of UnderLyingType - 1");
            static_assert(obj_len_bit <= sizeof(T) * CHAR_BIT, "obj_len_bit > actual size of object");
            public:
            using UnderLyingType = UnderLyingType_;
            static const std::size_t Width = W, Height = H;
            static const std::size_t BITS_ALL = W * H * obj_len_bit;
            static const std::size_t BUF_LEN_IN_UNDERLYING_TYPE = BITS_ALL / (sizeof(UnderLyingType) * CHAR_BIT) + 1;
            static const std::size_t BUF_LEN = BUF_LEN_IN_UNDERLYING_TYPE * sizeof(UnderLyingType);
            static const UnderLyingType MASK = (1ULL << obj_len_bit) - 1;
            using ObjectType = T;
            using PointType = GridPoint<W, H>;
            private:
            UnderLyingType buf[BUF_LEN_IN_UNDERLYING_TYPE];

            template<typename B, typename A>
            static B noalias_cast(A a) {
                union N {
                    A a;
                    B b;
                    N(A a):a(a) { }
                };
                return N(a).b;
            }

            const unsigned char *buf_as_puchar() const
            {
                return reinterpret_cast<const unsigned char *>(buf);
            }
            unsigned char *buf_as_puchar()
            {
                return reinterpret_cast<unsigned char *>(buf);
            }
            public:
            CompressedGrid()
            {
                clear();
            }

            CompressedGrid(const T& init_val)
            {
                fill(init_val);
            }

            static std::size_t getBitIndex(PointType p)
            {
                return obj_len_bit * (p.x * W + p.y);
            }
            static std::pair<std::size_t, std::size_t> getByteBitOffset(std::size_t bitIndex) // bitIdx -> (ByteIdx, bitOffsetInCell)
            {
                return std::make_pair(bitIndex / CHAR_BIT, bitIndex % CHAR_BIT);
            };
            UnderLyingType getRawbits(std::size_t bitIndex) const
            {
                const std::size_t charOffset = bitIndex / CHAR_BIT;
                const std::size_t bitOffset = bitIndex % CHAR_BIT;
                const unsigned char* const char_ptr= buf_as_puchar() + charOffset;
                const UnderLyingType *const longlong_ptr = reinterpret_cast<const UnderLyingType* const>(char_ptr);
                return (*longlong_ptr & (MASK << bitOffset)) >> bitOffset;
            }
            void setRawbits(std::size_t bitIndex, UnderLyingType data)
            {
                const std::size_t charOffset = bitIndex / CHAR_BIT;
                const std::size_t bitOffset = bitIndex % CHAR_BIT;
                unsigned char* const char_ptr= buf_as_puchar() + charOffset;
                UnderLyingType *const longlong_ptr = reinterpret_cast<UnderLyingType *const>(char_ptr);
                *longlong_ptr &= ~(MASK << bitOffset);
                *longlong_ptr |= (data & MASK) << bitOffset;
            }
            T get(PointType p) const
            {
                const std::size_t bitIndex = getBitIndex(p);
                UnderLyingType u = getRawbits(bitIndex);
                return noalias_cast<T>(u);
            }
            void set(PointType p, ObjectType obj)
            {
                const std::size_t bitIndex = getBitIndex(p);
                UnderLyingType ull = 0;
                new (&ull) ObjectType(obj);
                setRawbits(bitIndex, ull);
            }
            void clear()
            {
                std::memset(buf, 0, sizeof(buf));
            }

            void fill(const T& init_val)
            {
                for (PointType p {0, 0}; p.x < W; ++p)
                    set(p, init_val);
            }

            UnderLyingType *raw()
            {
                return buf;
            }
            const UnderLyingType *raw() const
            {
                return buf;
            }

        private:
            template<typename F1T, typename F2T>
            void inner_join(const CompressedGrid &other, F1T f1, F2T f2)
            {
                UnderLyingType* it1= raw();
                const UnderLyingType *it2 = other.raw();

                for (UnderLyingType i=0; i<BUF_LEN_IN_UNDERLYING_TYPE; ++i)
                {
                    f1(it1[i], it2[i]);
                }
            }
        public:

            std::size_t count() const
            {
                std::size_t ans = 0;
                for (std::size_t i = 0; i < BUF_LEN_IN_UNDERLYING_TYPE; ++i)
                {
                    ans += cg_popcount(buf[i]);
                }
                return ans;
            }

            CompressedGrid operator | (const CompressedGrid &other) const
            {
                CompressedGrid a = *this;
                a |= other;
                return a;
            }
            CompressedGrid &operator |= (const CompressedGrid& other)
            {
                if (&other == this) return *this;
                inner_join(other, [](UnderLyingType &a, UnderLyingType b) {
                    a |= b;
                }, [](unsigned char &a, unsigned char b) {
                    a |= b;
                });
                return *this;
            }
            CompressedGrid operator & (const CompressedGrid &other) const
            {
                CompressedGrid a = *this;
                a &= other;
                return a;
            }
            CompressedGrid &operator &= (const CompressedGrid& other)
            {
                if (&other == this) return *this;
                inner_join(other, [](UnderLyingType &a, UnderLyingType b) {
                    a &= b;
                }, [](unsigned char &a, unsigned char b) {
                    a &= b;
                });
                return *this;
            }
            CompressedGrid operator ^ (const CompressedGrid &other) const {
                CompressedGrid a = *this;
                a ^= other;
                return a;
            }
            CompressedGrid &operator ^= (const CompressedGrid& other)
            {
                if (&other == this) return *this;
                inner_join(other, [](UnderLyingType &a, UnderLyingType b) {
                    a ^= b;
                }, [](unsigned char &a, unsigned char b) {
                    a ^= b;
                });
                return *this;
            }
        };

}

namespace std
{
    template<typename T, std::size_t W, std::size_t H, std::size_t OBJLEN, typename UT> struct hash<compgrid::CompressedGrid<T, W, H, OBJLEN, UT> >
    {
        private:
            using F = compgrid::CompressedGrid<T, W, H, OBJLEN, UT>;
        public:
            std::size_t operator() (const F& cg) const
            {
                std::size_t ans = 0;

                for (std::size_t i = 0; i < F::BUF_LEN_IN_UNDERLYING_TYPE; ++i)
                {
                    ans ^= cg.raw()[i];
                } 
                return ans;
            }
    };
}
#endif
