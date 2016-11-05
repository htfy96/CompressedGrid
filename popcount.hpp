//
// Created by lz on 11/5/16.
//

#ifndef COMPRESSEDGRID_POPCOUNT_HPP_HPP
#define COMPRESSEDGRID_POPCOUNT_HPP_HPP
// Workaround for msvc
#if defined(__MSC_VER)
#  include <intrin.h>
#  define cg_popcount __popcntll
#else
#  define cg_popcount __builtin_popcountll
#endif
#endif //COMPRESSEDGRID_POPCOUNT_HPP_HPP
