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

    static_assert(std::is_pod<GridPoint<1,2> >::value, "Grid Point is POD!");

    template<typename T, std::size_t W, std::size_t H, std::size_t obj_len_bit, typename UnderLyingType = unsigned long long>
        class CompressedGrid
        {
            static_assert(obj_len_bit > 0 && obj_len_bit <= (sizeof(UnderLyingType) - 1) * CHAR_BIT,
                        // if obj_len_bit == 64(8 bytes), its read may across 9 aligned bytes!
                        "Object too large to be stored (> size of UnderLyingType - 1");
            static_assert(obj_len_bit <= sizeof(T) * CHAR_BIT, "obj_len_bit > actual size of object");
            static_assert(std::is_pod<T>::value, "T must be POD!");
            public:
            static const std::size_t Width = W, Height = H;
            static const std::size_t BITS_ALL = W * H * obj_len_bit;
            static const std::size_t BUF_LEN = (BITS_ALL + CHAR_BIT - 1) / CHAR_BIT + sizeof(unsigned long long);
            static const UnderLyingType MASK = (1ULL << obj_len_bit) - 1;
            using ObjectType = T;
            using PointType = GridPoint<W, H>;
            private:
            unsigned char buf[BUF_LEN];
            public:
            CompressedGrid()
            {
                clear();
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
                const unsigned char* const char_ptr= buf + charOffset;
                const UnderLyingType *const longlong_ptr = reinterpret_cast<const UnderLyingType* const>(char_ptr);
                return (*longlong_ptr & (MASK << bitOffset)) >> bitOffset;
            }
            void setRawbits(std::size_t bitIndex, UnderLyingType data)
            {
                const std::size_t charOffset = bitIndex / CHAR_BIT;
                const std::size_t bitOffset = bitIndex % CHAR_BIT;
                unsigned char* const char_ptr= buf + charOffset;
                UnderLyingType *const longlong_ptr = reinterpret_cast<UnderLyingType *const>(char_ptr);
                *longlong_ptr &= ~(MASK << bitOffset);
                *longlong_ptr |= (data & MASK) << bitOffset;
            }
            T get(PointType p) const
            {
                const std::size_t bitIndex = getBitIndex(p);
                UnderLyingType obj_raw = getRawbits(bitIndex);
                return reinterpret_cast<T&>(obj_raw);
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
                memset(buf, 0, sizeof(buf));
            }

            unsigned char *raw()
            {
                return buf;
            }
            const unsigned char* raw() const
            {
                return buf;
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
            std::size_t operator() (const F& cg)
            {
                std::size_t ans = 0;
                const std::size_t* it= reinterpret_cast<const std::size_t*>(cg.raw());
                std::size_t count = F::BUF_LEN / sizeof(std::size_t), remain = F::BUF_LEN % sizeof(std::size_t);

                for (std::size_t i = 0; i < count; ++i)
                {
                    ans ^= it[i];
                } 
                const unsigned char* tail_ptr = reinterpret_cast<const unsigned char*>(it + count);
                unsigned char off = 0;
                for (const unsigned char* tailit = tail_ptr; tailit != tail_ptr + remain; ++tailit)
                {
                    ans ^= *tailit << off;
                    off += CHAR_BIT;
                }
                return ans;
            }
    };
}
#endif
