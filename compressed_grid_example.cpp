#include "compressed_grid.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <cassert>


// Performance:
// All with g++ 6.2.1, -O2 CompressedGrid<int, 19, 19, 11> on x86-64 ArchLinux i7-4770HQ
//
// Random set & get 100M times:
//  Compressed Grid: 3253ms
//  Raw array: 3110ms
//
//  Result corectness checked
//
// Memory Size:
//  Compressed Grid: 520 B
//  Raw Array: 1444 B
//
//
// 10M random read/write:
//  Check passed
//  No hash collision
int main()
{
    using namespace std;
    using namespace compgrid;
    std::srand(time(0));
    CompressedGrid<int, 19, 19, 21> cg;
    using cg_t = decltype(cg);
    cg.set(cg_t::PointType { 2, 3}, 17);
    cg.set(cg_t::PointType { 2, 4}, 1022);
    cout << cg.get(  cg_t::PointType {2, 3} ) << " " << cg.get(cg_t::PointType{2, 4}) << endl;
    cout << cg.get( cg_t::PointType{7, 5});
    vector<int> t;
    cout << endl;
    const int IT_CNT = 8000000;
    t.reserve(IT_CNT);
    auto start = chrono::high_resolution_clock::now();
    for (int i=0; i<IT_CNT; ++i)
    {
        char x = rand() % 19;
        char y = rand() % 19;
        if (rand() % 2) cg.set(cg_t::PointType{ x, y }, rand() % 1024);
        else t.push_back(cg.get(cg_t::PointType { x, y }));
    }
    auto diff = chrono::high_resolution_clock::now() - start;
    cout << "Time of CompressedGrid:" <<  chrono::duration_cast<chrono::milliseconds>(diff).count() << "ms" << endl;

    cout << t[261] << endl;

    int a[19][19];
    memset(a, 0, sizeof(a));
    t.clear();
    start = chrono::high_resolution_clock::now();
    for (int i=0; i<IT_CNT; ++i)
    {
        char x = rand() % 19, y = rand() % 19;
        if (rand() % 2) a[x][y] = rand() % 1024;
        else 
            t.push_back(a[x][y]);
    }

    diff = chrono::high_resolution_clock::now() - start;
    cout << "Time of raw array:" << chrono::duration_cast<chrono::milliseconds>(diff).count() << "ms" << endl;

    cout << t.size() << " " << t[t.size() - 1] << endl;

    cout << "Size of raw array:"
        << sizeof(a) << " size of CompressedGrid:" << sizeof(cg) << endl;

    memset(a, 0, sizeof(a));
    cg.clear();

    std::hash<cg_t> h;
    size_t last = h(cg);

    cout << "Checking correctness & hash collision..." << endl;
    for (int i=0; i<IT_CNT; ++i)
    {
        int x = rand() % 19, y = rand() % 19;
        bool op = rand() % 2;
        if (op)
        {
            int val;
            do 
            {
                val = rand() % 1024;
            } while(a[x][y] == val);

            a[x][y] = val;
            cg.set(cg_t::PointType {char(x), char(y)}, val);
            size_t cur = h(cg);
            if (cur == last)
                cout << "Hash collision!" << endl;
            last = cur;
            /*
               cout <<
               "[" << i << "] " <<
               "Set " << x << " " << y << " " << val << endl;
               */
        } else
        {
            int real = a[x][y];
            int actual = cg.get(cg_t::PointType{ char(x), char(y) } );
            /*
               cout << 
               "[" << i << "] " << 

               "x=" << x << " y=" << y << " Real=" << real << " Actual="<< actual << endl;
               */
            if (real != actual)
                cout << "!!!!!!!!!!!! comparison fails!" << endl;

        }
    }

    cout << "Checking bitset correctness" << endl;
    cg.clear();
    memset(a, 0, sizeof(a));
    assert(0 == cg.count());

    a[0][0] = 157;
    cg.set(cg_t::PointType {0, 0}, 157);

    assert(cg.get(cg_t::PointType{ 0, 0}) == 157);
    cout << cg.count() << endl;
    assert(cg_popcount(a[0][0]) == cg.count());


    for (int i=0; i<IT_CNT; ++i)
    {
        int x = rand() % 19, y = rand() % 19;
        int val;
        do 
        {
            val = rand() % 1024;
        } while(a[x][y] == val);

        //cout << "Set " << x <<" "<< y <<" to " << val << endl;
        a[x][y] = val;
        cg.set(cg_t::PointType {char(x), char(y)}, val);
        assert(cg.get(cg_t::PointType{ char(x), char(y) }) == a[x][y]);
        int cg_count = cg.count();
        int a_count = 0;
        for (int i=0 ;i<19; ++i)
            for (int j=0; j<19; ++j)
                a_count += cg_popcount(a[i][j]);
        if (cg_count != a_count)
        {
            cout << "Count failed! Expected: "<< a_count << " Actual: "<< cg_count << endl;
            //system("read");
        }
    }

    cout << "Checking |= correctness" << endl;
    cg.clear();
    memset(a, 0, sizeof(a));

    decltype(cg) cg2;
    int b[19][19];
    memset(b, 0, sizeof(b));
    std::cout << sizeof(b) << std::endl;

    for (int i=0; i<360; ++i)
    {
        int x = rand() % 19, y = rand() % 19;
        int val;
        do 
        {
            val = rand() % 1024;
        } while(a[x][y] == val);
        a[x][y] = val;
        cg.set(cg_t::PointType {char(x), char(y)}, val);
        assert(cg.get(cg_t::PointType {char(x), char(y)}) == val);
    }

    for (int i=0; i<360; ++i)
    {
        int x = rand() % 19, y = rand() % 19;
        int val;
        do 
        {
            val = rand() % 1024;
        } while(b[x][y] == val);
        b[x][y] = val;
        cg2.set(cg_t::PointType {char(x), char(y)}, val);
        assert(cg2.get(cg_t::PointType {char(x), char(y)}) == val);
    }

    assert(cg.get(cg_t::PointType{ char(0), char(0) }) == a[0][0]);

    std::size_t expected_cnt = 0;
    cg |= cg2;
    for (int i=0; i<19; ++i)
        for (int j=0; j<19; ++j)
        {
            int before_a = a[i][j], before_b = b[i][j];
            a[i][j] |= b[i][j];
            expected_cnt += cg_popcount(a[i][j]);
            int actual = cg.get(cg_t::PointType { char(i), char(j) }), expected = a[i][j];
            if (actual != expected)
            {
                cout << "Or failed! Expected:" << expected << " Actual:" << actual << " at " <<i << ", " << j  <<
                     "  before: a=" << before_a << " b=" << before_b << std::endl;
            }
        }
    std::size_t actual_cnt = cg.count();

    if (expected_cnt != actual_cnt)
        cout << "Count after |= failed! Expected:" << expected_cnt  << " Actual:" << actual_cnt << endl;
    

    cout << "done!" << endl;
}
