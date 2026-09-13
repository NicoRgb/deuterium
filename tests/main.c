int is_even(int n)
{
    int res = 1;
    if (n % 2)
        res = 0;
    else
        res = 2;

    return res;
}
