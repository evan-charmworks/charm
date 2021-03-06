#include "tram3d.decl.h"
#include <algorithm>
#include <climits>
#include <random>
#include <vector>
CProxy_main master; //readonly
class main : public CBase_main
{
  CProxy_Test blocks;
  int N;

 public:
  main(CkArgMsg* args)
  {
    N = 2;
    CkArrayOptions opts;
    opts.setBounds(N, N, N);
    blocks = CProxy_Test::ckNew(opts);
    std::mt19937 engine(37);  // arbitrarily selected constant seed for reproducibility
    std::uniform_int_distribution<> distro(INT_MIN, INT_MAX);
    master = thisProxy;
    for (int i = 0; i != N; ++i)
    {
      for (int j = 0; j != N; ++j)
      {
        for (int k = 0; k != N; ++k)
        {
          blocks(i, j, k).insert(distro(engine), N);
        }
      }
    }
    blocks.doneInserting();
    blocks.run();
    delete args;
  }
  void endexec(int val)
  {
    /* The test has 4 phases: initialization, distribution, and
    reduction.

    In phase 1, the main thread generates a sequence of starter
    values for each element of the array, using a well-defined
    constant as the initial seed. Each thread stores a series of
    N*N*N values generated by incrementing the starter values they
    received.

    In phase 2, all the threads redistribute their values by sending
    them to elements of the array based on their indices(including
    themselves).

    In phase 3, after all the threads have received the new values,
    they contribute the minimum of all the values they received into
    a sum-reduction.

    The value below is obtained by running the test code without
    TRAM enabled, with the same deterministic seed.
    */
    if (val != 488803188)
    {
      CkAbort("Messages not delivered correctly!");
    }
    else
    {
      CkPrintf("The sum of minimal values across chares is %d\n", val);
      CkExit();
    }
  }
};
class Test : public CBase_Test
{
  Test_SDAG_CODE
  std::vector<int> values;
  std::vector<int> recvd;
  int N;
  int count1, count2, count3;

 public:
  Test() {}
  Test(int seed, int N)
      : values([this, seed, N]() mutable {
          std::vector<int> temp;
          temp.reserve(N * N * N);
          std::generate_n(std::back_inserter(temp), N * N * N,
                          [seed]() mutable { return seed++; });
          return temp;
        }()),
        N(N),
        count1(0),
        count2(0),
        count3(0)
  {
    recvd.reserve(N * N * N);
  }
};
#include "tram3d.def.h"
