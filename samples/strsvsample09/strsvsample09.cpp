/*
Copyright(c) 2002-2017 Anatoliy Kuznetsov(anatoliy_kuznetsov at yahoo.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

For more information please visit:  http://bitmagic.io
*/

/** \example strsvsample09.cpp
  Example of how to use bm::str_sparse_vector<> - succinct container for
  bit-transposed string collections
 
  \sa bm::str_sparse_vector
*/

/*! \file strsvsample09.cpp
    \brief Example: str_sparse_vector<> swap example
*/

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <algorithm>

#include "bm.h"
#include "bmstrsparsevec.h"
//#include "bmsparsevec_algo.h"

#include "bmdbg.h"
#include "bmtimer.h"

#include "bmundef.h" /* clear the pre-proc defines from BM */

using namespace std;

typedef bm::bvector<> bvector_type;

// define the sparse vector type for 'char' type using bvector as
// a container of bits for bit-transposed planes
typedef bm::str_sparse_vector<char, bvector_type, 16> str_sv_type;


/// generate collection of strings from integers with common prefixes
/// ... and shuffle it
static
void generate_string_set(vector<string>& str_vec,
                         const unsigned max_coll = 250000)
{
    str_vec.resize(0);
    string str;
    for (unsigned i = 10; i < max_coll; i += unsigned(rand() % 3))
    {
        switch (rand()%2)
        {
        case 0: str = "nssv"; break;
        default: str = "rs"; break;
        }

        str.append(to_string(i));
        str_vec.emplace_back(str);
    } // for i
    
    std::random_device rd;
    std::mt19937       g(rd());
    std::shuffle(str_vec.begin(), str_vec.end(), g);
}



void quicksort(str_sv_type& strsv, int first, int last)
{
    using stype = str_sv_type::size_type;
    int i, j, pivot;
    std::string s1, s2;

    if (first<last)
    {
        pivot = i= first;
        j = last;
        while (i <j)
        {
            while((i < last) && (strsv.compare(stype(i), stype(pivot)) <= 0))
                i++;
            while(strsv.compare(stype(j), stype(pivot)) > 0) // number[j]>number[pivot])
                j--;
            if (i < j)
            {
                strsv.get(stype(i), s1);
                strsv.get(stype(j), s2);
                strsv.assign(stype(i), s2);
                strsv.assign(stype(j), s1);
            }
        } // while
        strsv.get(stype(pivot), s1);
        strsv.get(stype(j), s2);
        strsv.assign(stype(pivot), s2);
        strsv.assign(stype(j), s1);

        quicksort(strsv, first, j-1);
        quicksort(strsv, j+1, last);
    }
}

void quicksort2(str_sv_type& strsv, int first, int last)
{
    using stype = str_sv_type::size_type;
    int i, j, pivot;

    str_sv_type::value_type pivot_buf[128]; // fixed buffer for simplicity
    if (first<last)
    {
        pivot = i= first;
        j = last;

        strsv.get(stype(pivot), pivot_buf, sizeof(pivot_buf));
        
        while (i <j)
        {
            while((i < last) && (strsv.compare(stype(i), pivot_buf) <= 0))
                i++;
            while(strsv.compare(stype(j), pivot_buf) > 0)
                j--;
            if (i < j)
                strsv.swap(stype(i), stype(j));
        } // while
        strsv.swap(stype(pivot), stype(j));

        quicksort(strsv, first, j-1);
        quicksort(strsv, j+1, last);
    }
}





bm::chrono_taker<>::duration_map_type  timing_map;

int main(void)
{
    try
    {
        str_sv_type str_sv, str_sv2;

        vector<string> str_vec;
        generate_string_set(str_vec);

        // load compact vector
        cout << "Loading " << str_vec.size() << " elements..." << endl;
        {
            auto bi = str_sv.get_back_inserter();
            for (const string& term : str_vec)
                bi = term;
            bi.flush();
        }
        // remap succinct vector into optimal codes
        // (after final load of content)
        //
        str_sv.remap();
        {
            BM_DECLARE_TEMP_BLOCK(tb)
            str_sv.optimize(tb);
        }
        str_sv2 = str_sv;

        cout << "Quick Sort..." << endl;

        {
        bm::chrono_taker tt1(cout, "1. quick sort (succint)", 1, &timing_map);
        quicksort(str_sv, 0, (int)str_sv.size()-1);
        }

        {
        bm::chrono_taker tt1(cout, "2. quick sort 2 (succint)", 1, &timing_map);
        quicksort2(str_sv2, 0, (int)str_sv2.size()-1);
        }


        {
        bool eq = str_sv.equal(str_sv2);
        if (!eq)
        {
            cerr << "post-sort vector mismatch!" << endl;
            assert(0); exit(1);
        }
        }




        // validate the results to match STL sort
        cout << "std::sort..." << endl;
        {
        bm::chrono_taker tt1(cout, "3. std::sort()", 1, &timing_map);
        std::sort(str_vec.begin(), str_vec.end());
        }

        {
            vector<string>::const_iterator sit = str_vec.begin();
            str_sv_type::const_iterator it = str_sv.begin();
            str_sv_type::const_iterator it_end = str_sv.end();
            for (; it != it_end; ++it, ++sit)
            {
                string s = *it;
                if (*sit != s)
                {
                    cerr << "Mismatch at:" << s << "!=" << *sit << endl;
                    return 1;
                }
            } // for
        }
        cout << "Sort validation Ok." << endl;

        bm::chrono_taker<>::print_duration_map(cout, timing_map, bm::chrono_taker<>::ct_time);

    }
    catch(std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    

    return 0;
}

