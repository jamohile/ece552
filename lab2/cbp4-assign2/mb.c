// easy micro benchmark
#define ITERS 10000

// Testcase 1:
// When there is no content in the loop, 1943 mispredicts.
int main()
{
    unsigned int i = 0;
    unsigned int b = 0;

    // This outer while branches (taken) to continue.
    //  It "not takens" only at the end.
    //  Therefore: it should contribute only one mispredict.

    while (i < ITERS)
    {
        i += 1;
        //  it should mispredict 6
        if (i % 7 == 0)
        {
            b = 1;
        }
    }

    return 0;
}
