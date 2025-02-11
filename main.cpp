﻿// 这是第07课的回家作业，主题是访存优化
// 录播见：https://www.bilibili.com/video/BV1gu41117bW
// 作业的回答推荐写在 ANSWER.md 方便老师批阅，也可在 PR 描述中
// 请晒出程序被你优化前后的运行结果（ticktock 的用时统计）
// 可以比较采用了不同的优化手段后，加速了多少倍，做成表格
// 如能同时贴出 CPU 核心数量，缓存大小等就最好了（lscpu 命令）
// 作业中有很多个问句，请通过注释回答问题，并改进其代码，以使其更快
// 并行可以用 OpenMP 也可以用 TBB
#include <cassert>
#include <emmintrin.h>
#include <iostream>
#include <stdio.h>
#include <cstring>
// #include <x86intrin.h>  // _mm 系列指令都来自这个头文件
#include <xmmintrin.h>  // 如果上面那个不行，试试这个
#include "morton.h"
#include "ndarray.h"
#include "wangsrng.h"
#include "ticktock.h"

union Whatever {
                int int_type;
                float float_type;
};

constexpr size_t blockSize = 64;
// Matrix 是 YX 序的二维浮点数组：mat(x, y) = mat.data()[y * mat.shape(0) + x]
using Matrix = ndarray<2, float, 0, blockSize, AlignedAllocator<float, 4096>>;
// 注意：默认对齐到 64 字节，如需 4096 字节，请用 ndarray<2, float, AlignedAllocator<4096, float>>

static void matrix_randomize(Matrix &out) {
    TICK(matrix_randomize);
    size_t nx = out.shape(0);
    size_t ny = out.shape(1);

    // 这个循环为什么不够高效？如何优化？ 10 分
#pragma omp parallel for
    for (size_t y = 0; y < ny; y++) {
        for (size_t x = 0; x < nx; x++) {
            Whatever wa;
            wa.float_type = wangsrng(x, y).next_float();
            _mm_stream_si32((int*) &out(x, y), wa.int_type);
        }
    }
    TOCK(matrix_randomize);
}

static void matrix_transpose(Matrix &out, Matrix const &in) {
    TICK(matrix_transpose);
    size_t nx = in.shape(0);
    size_t ny = in.shape(1);
    out.reshape(ny, nx);

    // 这个循环为什么不够高效？如何优化？ 15 分

#pragma omp parallel for
    for (size_t mortonCode = 0; mortonCode < (ny / blockSize) * (nx / blockSize); mortonCode++) {
        auto [xBase, yBase] = morton2d::decode(mortonCode);
        xBase *= blockSize;
        yBase *= blockSize;
        if(xBase>= nx|| yBase>= ny){ continue; }
        // assert(xBase < nx);
        for (size_t x = xBase; x < xBase + blockSize; x++) {
            for (size_t y = yBase; y < yBase + blockSize; y++) {
                Whatever wa;
                wa.float_type = in(x, y);
                _mm_stream_si32((int *) &out(y, x), wa.int_type);
            }
        }
    }
    TOCK(matrix_transpose);
}

static void matrix_transpose2(Matrix &out, Matrix const &in) {
    TICK(matrix_transpose);
    size_t nx = in.shape(0);
    size_t ny = in.shape(1);
    out.reshape(ny, nx);
#pragma omp parallel for collapse(2)
    for (size_t x = 0; x < nx; x++) {
        for (size_t y = 0; y < ny; y++) {
            out(y, x) = in(x, y);
        }
    }
    TOCK(matrix_transpose);
}

static void matrix_multiply(Matrix &out, Matrix const &lhs, Matrix const &rhs) {
    TICK(matrix_multiply);
    size_t nx = lhs.shape(0);
    size_t nt = lhs.shape(1);
    size_t ny = rhs.shape(1);
    if (rhs.shape(0) != nt) {
        std::cerr << "matrix_multiply: shape mismatch" << std::endl;
        throw;
    }
    out.reshape(nx, ny);
    // 这个循环为什么不够高效？如何优化？ 15 分
#pragma omp parallel for
    for (size_t y = 0; y < ny; y++) {
        for (size_t iBase = 0; iBase < nx; iBase += 32) {
            // out(x, y) = 0;  // 有没有必要手动初始化？ 5 分

            // 不需要初始化
            // 最后执行会对每个值得地址执行 ::new(reinterpret_cast<void*>(p)) float(); }
            // float a = float();
            // printf("%e", a); //0.000000e+00
            // 所有元素都被清空

            // 调用堆栈
            // AlignedAllocator<...>::construct<float>(AlignedAllocator<float, 4096> * const this, float * p) (d:\cpp\hw07\alignalloc.h:105)
            // ...
            // std::__uninitialized_default_n_a<...>(float * __first, unsigned long long __n, ... & __alloc) (d:\TDM-GCC-64\lib\gcc\x86_64-w64-mingw32\10.3.0\include\c++\bits\stl_uninitialized.h:670)
            // std::vector<...>::_M_default_append(std::vector<...> * const this, std::vector<...>::size_type __n) (d:\TDM-GCC-64\lib\gcc\x86_64-w64-mingw32\10.3.0\include\c++\bits\vector.tcc:627)
            // std::vector<...>::resize(std::vector<...> * const this, std::vector<...>::size_type __new_size) (d:\TDM-GCC-64\lib\gcc\x86_64-w64-mingw32\10.3.0\include\c++\bits\stl_vector.h:940)
            // ndarray<2ull, float>::reshape(ndarray<2, float> * const this, Shape & shape) (d:\cpp\hw07\ndarray.h:60)
            // ndarray<2ull, float>::reshape<unsigned long long, unsigned long long, 0>(ndarray<2, float> * const this) (d:\cpp\hw07\ndarray.h:79)
            // ...


            for (size_t t = 0; t < nt; t++) {
                for (size_t x = iBase; x < iBase + 32; x++) {
                    out(x, y) += lhs(x, t) * rhs(t, y);
                }
            }
        }
    }
    TOCK(matrix_multiply);
}

static void matrix_multiply2(Matrix &out, Matrix const &lhs, Matrix const &rhs) {
    TICK(matrix_multiply);
    size_t nx = lhs.shape(0);
    size_t nt = lhs.shape(1);
    size_t ny = rhs.shape(1);
    if (rhs.shape(0) != nt) {
        std::cerr << "matrix_multiply: shape mismatch" << std::endl;
        throw;
    }
    out.reshape(nx, ny);
#pragma omp parallel for collapse(2)
    for (size_t y = 0; y < ny; y++) {
        for (size_t x = 0; x < nx; x++) {
            out(x, y) = 0;  // 有没有必要手动初始化？ 5 分
            for (size_t t = 0; t < nt; t++) {
                out(x, y) += lhs(x, t) * rhs(t, y);
            }
        }
    }
    TOCK(matrix_multiply);
}

// 求出 R^T A R
static void matrix_RtAR(Matrix &RtAR, Matrix const &R, Matrix const &A) {
    TICK(matrix_RtAR);
    // 这两个是临时变量，有什么可以优化的？ 5 分
    Matrix Rt, RtA, RtB;
    matrix_transpose(Rt, R);

    // matrix_transpose2(RtA, Rt);
    // printf("cmp transpose: %d\n", memcmp(RtA.data(), R.data(), sizeof(float) * sizeof (R.data())));

    // matrix_randomize(RtA); 用于测试  “有没有必要手动初始化”
    matrix_multiply(RtA, Rt, A);
    // matrix_multiply2(RtB, Rt, A);
    // printf("cmp multiply: %d\n", memcmp(RtA.data(), RtB.data(), sizeof(float) * sizeof (RtB.data())));
    matrix_multiply(RtAR, RtA, R);
    TOCK(matrix_RtAR);
}

static float matrix_trace(Matrix const &in) {
    TICK(matrix_trace);
    float res = 0;
    size_t nt = std::min(in.shape(0), in.shape(1));
#pragma omp parallel for reduction(+:res)
    for (size_t t = 0; t < nt; t++) {
        res += in(t, t);
    }
    TOCK(matrix_trace);
    return res;
}

static void test_func(size_t n) {
    TICK(test_func);
    Matrix R(n, n);
    matrix_randomize(R);
    Matrix A(n, n);
    matrix_randomize(A);

    Matrix RtAR;
    matrix_RtAR(RtAR, R, A);

    // std::cout << matrix_trace(RtAR) << std::endl;
    TOCK(test_func);
}

int main() {
    wangsrng rng;
    TICK(overall);
    for (int t = 0; t < 4; t++) {
        size_t n = 64 * (rng.next_uint64() % 16 + 24);
        std::cout << "t=" << t << ": n=" << n << std::endl;
        test_func(n);
    }
    TOCK(overall);
    return 0;
}
