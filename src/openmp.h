/*
 * Copyright (C) 2009-2013 Christoph L. Spiel
 *
 * This file is part of Enblend.
 *
 * Enblend is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Enblend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Enblend; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef OPENMP_H_INCLUDED_
#define OPENMP_H_INCLUDED_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <limits>

#include <vigra/diff2d.hxx>
#include <vigra/initimage.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/combineimages.hxx>
#include <vigra/convolution.hxx>
#include <vigra/distancetransform.hxx>


#if _OPENMP >= 200203 // at least OpenMP version 2.0

#include <omp.h>

#include "muopt.h"


#define OPENMP
#define OPENMP_YEAR (_OPENMP / 100)
#define OPENMP_MONTH (_OPENMP % 100)


#if defined(HAVE_ALLOCA_H)
#include <alloca.h>
#elif defined(__GNUC__)
#define alloca __builtin_alloca
#elif defined(_AIX)
#define alloca __alloca
#elif defined(_MSC_VER)
#include <malloc.h>
#define alloca _alloca
#else
#include <stddef.h>
#if defined(__cplusplus)
extern "C"
#endif
void* alloca(size_t);
#endif


namespace omp
{
    inline static void* malloc(size_t) __attribute__((alloc_size(1), malloc));

    inline static void*
    malloc(size_t size)
    {
#ifdef __ICC
        return kmp_malloc(size);
#else
        return alloca(size);
#endif
    }

    inline static void
    free(void* __attribute__((unused)) pointer)
    {
#ifdef __ICC
        return kmp_free(pointer);
#else
        ;                       // empty
#endif
    }


    class lock
    {
    public:
        lock() {omp_init_lock(&lock_);}
        ~lock() {omp_destroy_lock(&lock_);}

        void set() {omp_set_lock(&lock_);}
        void unset() {omp_unset_lock(&lock_);}
        bool test() {return omp_test_lock(&lock_);}

    private:
        lock(const lock&);            // not implemented
        lock& operator=(const lock&); // not implemented

        omp_lock_t lock_;
    };


    class nestable_lock
    {
    public:
        nestable_lock() {omp_init_nest_lock(&lock_);}
        ~nestable_lock() {omp_destroy_nest_lock(&lock_);}

        void set() {omp_set_nest_lock(&lock_);}
        void unset() {omp_unset_nest_lock(&lock_);}
        bool test() {return omp_test_nest_lock(&lock_);}

    private:
        nestable_lock(const nestable_lock&);            // not implemented
        nestable_lock& operator=(const nestable_lock&); // not implemented

        omp_nest_lock_t lock_;
    };
}


template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineTwoImagesMP(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                   SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                   DestImageIterator dest_upperleft, DestAccessor dest_acc,
                   const Functor& functor)
{
#pragma omp parallel
    {
        const vigra::Size2D size(src1_lowerright - src1_upperleft);
        Functor f(functor);

#pragma omp for schedule(guided) nowait
        for (int y = 0; y < size.y; ++y)
        {
            const vigra::Diff2D begin(0, y);
            const vigra::Diff2D end(size.x, y + 1);

            vigra::combineTwoImages(src1_upperleft + begin, src1_upperleft + end, src1_acc,
                                    src2_upperleft + begin, src2_acc,
                                    dest_upperleft + begin, dest_acc,
                                    f);
        }
    } // omp parallel
}


template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class MaskImageIterator, class MaskAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineTwoImagesIfMP(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                     SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                     MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                     DestImageIterator dest_upperleft, DestAccessor dest_acc,
                     const Functor& functor)
{
#pragma omp parallel
    {
        const vigra::Size2D size(src1_lowerright - src1_upperleft);
        Functor f(functor);

#pragma omp for schedule(guided) nowait
        for (int y = 0; y < size.y; ++y)
        {
            const vigra::Diff2D begin(0, y);
            const vigra::Diff2D end(size.x, y + 1);

            vigra::combineTwoImagesIf(src1_upperleft + begin, src1_upperleft + end, src1_acc,
                                      src2_upperleft + begin, src2_acc,
                                      mask_upperleft + begin, mask_acc,
                                      dest_upperleft + begin, dest_acc,
                                      f);
        }
    } // omp parallel
}


template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class SrcImageIterator3, class SrcAccessor3,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineThreeImagesMP(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                     SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                     SrcImageIterator3 src3_upperleft, SrcAccessor3 src3_acc,
                     DestImageIterator dest_upperleft, DestAccessor dest_acc,
                     const Functor& functor)
{
#pragma omp parallel
    {
        const vigra::Size2D size(src1_lowerright - src1_upperleft);
        Functor f(functor);

#pragma omp for schedule(guided) nowait
        for (int y = 0; y < size.y; ++y)
        {
            const vigra::Diff2D begin(0, y);
            const vigra::Diff2D end(size.x, y + 1);

            vigra::combineThreeImages(src1_upperleft + begin, src1_upperleft + end, src1_acc,
                                      src2_upperleft + begin, src2_acc,
                                      src3_upperleft + begin, src3_acc,
                                      dest_upperleft + begin, dest_acc,
                                      f);
        }
    } // omp parallel
}


template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
transformImageMP(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                 DestImageIterator dest_upperleft, DestAccessor dest_acc,
                 const Functor& functor)
{
#pragma omp parallel
    {
        const vigra::Size2D size(src_lowerright - src_upperleft);
        Functor f(functor);

#pragma omp for schedule(guided) nowait
        for (int y = 0; y < size.y; ++y)
        {
            const vigra::Diff2D begin(0, y);
            const vigra::Diff2D end(size.x, y + 1);

            vigra::transformImage(src_upperleft + begin, src_upperleft + end, src_acc,
                                  dest_upperleft + begin, dest_acc,
                                  f);
        }
    } // omp parallel
}


template <class SrcImageIterator, class SrcAccessor,
          class MaskImageIterator, class MaskAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
transformImageIfMP(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                   MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                   DestImageIterator dest_upperleft, DestAccessor dest_acc,
                   const Functor& functor)
{
#pragma omp parallel
    {
        const vigra::Size2D size(src_lowerright - src_upperleft);
        Functor f(functor);

#pragma omp for schedule(guided) nowait
        for (int y = 0; y < size.y; ++y)
        {
            const vigra::Diff2D begin(0, y);
            const vigra::Diff2D end(size.x, y + 1);

            vigra::transformImageIf(src_upperleft + begin, src_upperleft + end, src_acc,
                                    mask_upperleft + begin, mask_acc,
                                    dest_upperleft + begin, dest_acc,
                                    f);
        }
    } // omp parallel
}


namespace fh
{
namespace detail
{
    template <class ValueType>
    inline static ValueType
    square(ValueType x)
    {
        return x * x;
    }


    // Pedro F. Felzenszwalb, Daniel P. Huttenlocher
    // "Distance Transforms of Sampled Functions"


    template <class ValueType>
    struct ChessboardTransform1D
    {
        typedef ValueType value_type;

        int id() const {return 0;}

        void operator()(ValueType* /* RESTRICT d */, const ValueType* /* RESTRICT f */, int /* n */) const
        {
            vigra_fail("fh::detail::ChessboardTransform1D: not implemented");
        }
    };


    template <class ValueType>
    struct ManhattanTransform1D
    {
        typedef ValueType value_type;

        int id() const {return 1;}

        void operator()(ValueType* RESTRICT d, const ValueType* RESTRICT f, int n) const
        {
            const ValueType one = static_cast<ValueType>(1);

            d[0] = f[0];
            for (int q = 1; q < n; ++q)
            {
                d[q] = std::min<ValueType>(f[q], d[q - 1] + one);
            }
            for (int q = n - 2; q >= 0; --q)
            {
                d[q] = std::min<ValueType>(d[q], d[q + 1] + one);
            }
        }
    };


    template <class ValueType>
    struct EuclideanTransform1D
    {
        typedef ValueType value_type;

        int id() const {return 2;}

        void operator()(ValueType* RESTRICT d, const ValueType* RESTRICT f, int n) const
        {
            typedef float math_t;

            const math_t infinity = std::numeric_limits<math_t>::infinity();

            int* v = static_cast<int*>(omp::malloc(n * sizeof(int)));
            math_t* z = static_cast<math_t*>(omp::malloc((n + 1) * sizeof(math_t)));
            int k = 0;

            v[0] = 0;
            z[0] = -infinity;
            z[1] = infinity;

            for (int q = 1; q < n; ++q)
            {
                const math_t sum_q = static_cast<math_t>(f[q]) + square(static_cast<math_t>(q));
                math_t s = (sum_q - (f[v[k]] + square(v[k]))) / (2 * (q - v[k]));

                while (s <= z[k])
                {
                    --k;
                    // IMPLEMENTATION NOTE
                    //     Prefetching improves performance because we must iterate from high to
                    //     low addresses, i.e. against the cache's look-ahead algorithm.
                    HINTED_PREFETCH(z + k - 2U, PREPARE_FOR_READ, HIGH_TEMPORAL_LOCALITY);
                    s = (sum_q - (f[v[k]] + square(v[k]))) / (2 * (q - v[k]));
                }
                ++k;

                v[k] = q;
                z[k] = s;
                z[k + 1] = infinity;
            }

            k = 0;
            for (int q = 0; q < n; ++q)
            {
                while (z[k + 1] < static_cast<math_t>(q))
                {
                    ++k;
                }
                d[q] = square(q - v[k]) + f[v[k]];
            }

            omp::free(z);
            omp::free(v);
        }
    };


    template <class SrcImageIterator, class SrcAccessor,
              class DestImageIterator, class DestAccessor,
              class ValueType, class Transform1dFunctor>
    void
    fhDistanceTransform(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor sa,
                        DestImageIterator dest_upperleft, DestAccessor da,
                        ValueType background, Transform1dFunctor transform1d)
    {
        typedef typename Transform1dFunctor::value_type DistanceType;
        typedef typename vigra::NumericTraits<DistanceType> DistanceTraits;
        typedef vigra::BasicImage<DistanceType> DistanceImageType;

        const vigra::Size2D size(src_lowerright - src_upperleft);
        const int greatest_length = std::max(size.x, size.y);
        DistanceImageType intermediate(size, vigra::SkipInitialization);

#pragma omp parallel
        {
            DistanceType* const f = new DistanceType[greatest_length];
            DistanceType* const d = new DistanceType[greatest_length];

            DistanceType* const pf_end = f + size.y;
            const DistanceType* const pd_end = d + size.y;

// IMPLEMENTATION NOTE
//     We need "guided" schedule to reduce the waiting time at the
//     (implicit) barriers.  This holds true for the next OpenMP
//     parallelized "for" loop, too.
#pragma omp for schedule(guided)
            for (int x = 0; x < size.x; ++x)
            {
                SrcImageIterator si(src_upperleft + vigra::Diff2D(x, 0));
                for (DistanceType* pf = f; pf != pf_end; ++pf)
                {
                    *pf = EXPECT_RESULT(sa(si) == background, false) ? DistanceTraits::max() : DistanceTraits::zero();
                    ++si.y;
                }

                transform1d(d, f, size.y);

                typename DistanceImageType::column_iterator ci(intermediate.columnBegin(x));
                for (const DistanceType* pd = d; pd != pd_end; ++pd)
                {
                    *ci = *pd;
                    ++ci;
                    // IMPLEMENTATION NOTE
                    //     Prefetching about halves the number of stalls per instruction of this loop.
                    HINTED_PREFETCH(ci.operator->(), PREPARE_FOR_WRITE, HIGH_TEMPORAL_LOCALITY);
                }
            }

#pragma omp for nowait schedule(guided)
            for (int y = 0; y < size.y; ++y)
            {
                transform1d(d, &intermediate(0, y), size.x);
                DestImageIterator i(dest_upperleft + vigra::Diff2D(0, y));

                if (transform1d.id() == 2)
                {
                    for (DistanceType* pd = d; pd != d + size.x; ++pd, ++i.x)
                    {
                        da.set(sqrt(*pd), i);
                    }
                }
                else
                {
                    for (DistanceType* pd = d; pd != d + size.x; ++pd, ++i.x)
                    {
                        da.set(*pd, i);
                    }
                }
            }

            delete [] d;
            delete [] f;
        } // omp parallel
    }
} // namespace detail
} // namespace fh


template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class ValueType>
void
distanceTransformMP(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor sa,
                    DestImageIterator dest_upperleft, DestAccessor da,
                    ValueType background, int norm)
{
    switch (norm)
    {
    case 0:
        fh::detail::fhDistanceTransform(src_upperleft, src_lowerright, sa,
                                        dest_upperleft, da,
                                        background,
                                        fh::detail::ChessboardTransform1D<float>());
        break;

    case 1:
        fh::detail::fhDistanceTransform(src_upperleft, src_lowerright, sa,
                                        dest_upperleft, da,
                                        background,
                                        fh::detail::ManhattanTransform1D<float>());
        break;

    case 2: // FALLTHROUGH
    default:
        fh::detail::fhDistanceTransform(src_upperleft, src_lowerright, sa,
                                        dest_upperleft, da,
                                        background,
                                        fh::detail::EuclideanTransform1D<float>());
    }
}


#else


#undef OPENMP
#define OPENMP_YEAR 0
#define OPENMP_MONTH 0

inline void omp_set_num_threads(int) {}
inline int omp_get_num_threads() {return 1;}
inline int omp_get_max_threads() {return 1;}
inline int omp_get_thread_num() {return 0;}
inline int omp_get_num_procs() {return 1;}
inline void omp_set_dynamic(int) {}
inline int omp_get_dynamic() {return 0;}
inline int omp_in_parallel() {return 0;}
inline void omp_set_nested(int) {}
inline int omp_get_nested() {return 0;}


namespace omp
{
    inline static void* malloc(size_t) __attribute__((alloc_size(1), malloc));

    inline static void*
    malloc(size_t size)
    {
        return std::malloc(size);
    }

    inline static void
    free(void* pointer)
    {
        std::free(pointer);
    }


    class lock
    {
    public:
        lock() {}
        ~lock() {}

        void set() {}
        void unset() {}
        bool test() {return false;}

    private:
        lock(const lock&);            // not implemented
        lock& operator=(const lock&); // not implemented
    };


    class nestable_lock
    {
    public:
        nestable_lock() {}
        ~nestable_lock() {}

        void set() {}
        void unset() {}
        bool test() {return false;}

    private:
        nestable_lock(const nestable_lock&);            // not implemented
        nestable_lock& operator=(const nestable_lock&); // not implemented
    };
}


template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineTwoImagesMP(SrcImageIterator1 src1_upperleft,
                   SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                   SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                   DestImageIterator dest_upperleft, DestAccessor dest_acc,
                   const Functor& func)
{
    vigra::combineTwoImages(src1_upperleft, src1_lowerright, src1_acc,
                            src2_upperleft, src2_acc,
                            dest_upperleft, dest_acc,
                            func);
}


template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class MaskImageIterator, class MaskAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineTwoImagesIfMP(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                     SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                     MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                     DestImageIterator dest_upperleft, DestAccessor dest_acc,
                     const Functor& func)
{
    vigra::combineTwoImagesIf(src1_upperleft, src1_lowerright, src1_acc,
                              src2_upperleft, src2_acc,
                              mask_upperleft, mask_acc,
                              dest_upperleft, dest_acc,
                              func);
}


template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class SrcImageIterator3, class SrcAccessor3,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineThreeImagesMP(SrcImageIterator1 src1_upperleft, SrcImageIterator1 src1_lowerright, SrcAccessor1 src1_acc,
                     SrcImageIterator2 src2_upperleft, SrcAccessor2 src2_acc,
                     SrcImageIterator3 src3_upperleft, SrcAccessor3 src3_acc,
                     DestImageIterator dest_upperleft, DestAccessor dest_acc,
                     const Functor& func)
{
    vigra::combineThreeImages(src1_upperleft, src1_lowerright, src1_acc,
                              src2_upperleft, src2_acc,
                              src3_upperleft, src3_acc,
                              dest_upperleft, dest_acc,
                              func);
}


template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
transformImageMP(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                 DestImageIterator dest_upperleft, DestAccessor dest_acc,
                 const Functor& func)
{
    vigra::transformImage(src_upperleft, src_lowerright, src_acc,
                          dest_upperleft, dest_acc,
                          func);
}


template <class SrcImageIterator, class SrcAccessor,
          class MaskImageIterator, class MaskAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
transformImageIfMP(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                   MaskImageIterator mask_upperleft, MaskAccessor mask_acc,
                   DestImageIterator dest_upperleft, DestAccessor dest_acc,
                   const Functor& func)
{
    vigra::transformImageIf(src_upperleft, src_lowerright, src_acc,
                            mask_upperleft, mask_acc,
                            dest_upperleft, dest_acc,
                            func);
}


template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class ValueType>
void
distanceTransformMP(SrcImageIterator src_upperleft, SrcImageIterator src_lowerright, SrcAccessor src_acc,
                    DestImageIterator dest_upperleft, DestAccessor dest_acc,
                    ValueType background, int norm)
{
    vigra::distanceTransform(src_upperleft, src_lowerright, src_acc,
                             dest_upperleft, dest_acc,
                             background, norm);
}

#endif // _OPENMP >= 200505


// Answer whether the underlying OpenMP implementation really (thinks
// that it) supports nested parallelism.
inline static bool
have_openmp_nested()
{
    const bool openmp_nested = omp_get_nested();
    omp_set_nested(true);
    const bool result = omp_get_nested() != 0;
    omp_set_nested(openmp_nested);
    return result;
}


// Answer whether the underlying OpenMP implementation really (thinks
// that it) supports dynamic adjustment of the number of threads.
inline static bool
have_openmp_dynamic()
{
    const bool openmp_dynamic = omp_get_dynamic();
    omp_set_dynamic(true);
    const bool result = omp_get_dynamic() != 0;
    omp_set_dynamic(openmp_dynamic);
    return result;
}


//
// Argument Object Factory versions
//

template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineTwoImagesMP(vigra::triple<SrcImageIterator1, SrcImageIterator1, SrcAccessor1> src1,
                   vigra::pair<SrcImageIterator2, SrcAccessor2> src2,
                   vigra::pair<DestImageIterator, DestAccessor> dest,
                   const Functor& functor)
{
    combineTwoImagesMP(src1.first, src1.second, src1.third,
                       src2.first, src2.second,
                       dest.first, dest.second,
                       functor);
}


template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class MaskImageIterator, class MaskAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineTwoImagesIfMP(vigra::triple<SrcImageIterator1, SrcImageIterator1, SrcAccessor1> src1,
                     vigra::pair<SrcImageIterator2, SrcAccessor2> src2,
                     vigra::pair<MaskImageIterator, MaskAccessor> mask,
                     vigra::pair<DestImageIterator, DestAccessor> dest,
                     const Functor& functor)
{
    combineTwoImagesIfMP(src1.first, src1.second, src1.third,
                         src2.first, src2.second,
                         mask.first, mask.second,
                         dest.first, dest.second,
                         functor);
}


template <class SrcImageIterator1, class SrcAccessor1,
          class SrcImageIterator2, class SrcAccessor2,
          class SrcImageIterator3, class SrcAccessor3,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
combineThreeImagesMP(vigra::triple<SrcImageIterator1, SrcImageIterator1, SrcAccessor1> src1,
                     vigra::pair<SrcImageIterator2, SrcAccessor2> src2,
                     vigra::pair<SrcImageIterator3, SrcAccessor3> src3,
                     vigra::pair<DestImageIterator, DestAccessor> dest,
                     const Functor& functor)
{
    combineThreeImagesMP(src1.first, src1.second, src1.third,
                         src2.first, src2.second,
                         src3.first, src3.second,
                         dest.first, dest.second,
                         functor);
}


template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
transformImageMP(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                 vigra::pair<DestImageIterator, DestAccessor> dest,
                 const Functor& functor)
{
    transformImageMP(src.first, src.second, src.third,
                     dest.first, dest.second,
                     functor);
}


template <class SrcImageIterator, class SrcAccessor,
          class MaskImageIterator, class MaskAccessor,
          class DestImageIterator, class DestAccessor,
          class Functor>
inline void
transformImageIfMP(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                   vigra::pair<MaskImageIterator, MaskAccessor> mask,
                   vigra::pair<DestImageIterator, DestAccessor> dest,
                   const Functor& functor)
{
    transformImageIfMP(src.first, src.second, src.third,
                       mask.first, mask.second,
                       dest.first, dest.second,
                       functor);
}


template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class ValueType>
inline void
distanceTransformMP(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                    vigra::pair<DestImageIterator, DestAccessor> dest,
                    ValueType background, int norm)
{
    distanceTransformMP(src.first, src.second, src.third,
                        dest.first, dest.second,
                        background, norm);
}


namespace omp
{
    class scoped_nested
    {
    public:
        scoped_nested() : level_(omp_get_nested()) {}

        explicit scoped_nested(bool allow_nested) : level_(omp_get_nested())
        {
            omp_set_nested(static_cast<int>(allow_nested));
        }

        ~scoped_nested() {omp_set_nested(level_);}

    private:
        const int level_;
    };


    template <class basic_lock>
    class scoped_lock
    {
    public:
        scoped_lock(basic_lock& a_lock) : lock_(&a_lock) {lock_->set();}
        ~scoped_lock() {lock_->unset();}

        void set() {lock_->set();}
        void unset() {lock_->unset();}
        bool test() {return lock_->test();}

    private:
        scoped_lock();                              // not implemented
        scoped_lock(const scoped_lock&);            // not implemented
        scoped_lock& operator=(const scoped_lock&); // not implemented

        basic_lock* const lock_;
    }; // class scoped_lock
} // namespace omp


#endif // OPENMP_H_INCLUDED_

// Local Variables:
// mode: c++
// End:
