// easy micro benchmark
#define ITERS 1000000

int main()
{
    // this is a simple benchmark, I assume the predictor would have low mispredictions for the loop below because my history table has 6 bits
    int b;
    for (int z = 0; z < ITERS; z++)
    {
        for (int w = 0; w < 2; w++)
        {
            b = 1;
        }
    }
    // b = 2;
    // this is a more complex benchmark because I assume the predictor would have more mispredicts because in my second loop I assume all 6 bits of history bits would have to be used
    int a;
    for (int i = 0; i < ITERS; i++)
    {
        for (int j = 0; j < 10; j++)
            a = 10;
    }
    a = 15;
    return 0;
}