#include<stdio.h>
#include<stdlib.h>
#include "externFunc.h"
typedef int64_t ll;
void Var2File(ll x) {
    FILE *fp = NULL;
    fp = fopen("ASEO_DATA.txt", "w");
    fprintf(fp, "%ld", x^0xA4E0);
    fclose(fp);
}
ll File2Var() {
    FILE *fp = NULL;
    char buff[255];
    fp = fopen("ASEO_DATA.txt", "r");
    fscanf(fp, "%s", buff);
    fclose(fp);
    ll x = atoi(buff);
    return x^0xA4E0;
}
ll RSHash(ll x)
{
    char *str = (char*)calloc(10, sizeof(char));
    sprintf(str,"%ld",x);
    ll b = 378551;
    ll a = 63689;
    ll hash = 0;
    while (*str)
    {
        hash = hash * a + *str++;
        a *= b;
    }
    return (hash & 0x7FFFFFFF);
}
// p = 10000229; x = 2
ll quick_pow(ll n){
    ll x = 2;
    ll m = 10000229;
    ll res = 1;
    while(n > 0){
        if(n & 1)	res = res * x % m;
        x = x * x % m;
        n >>= 1;
    }
    return res;
}

ll rsa(ll m){
    //ll p = 10000121;
    //ll q = 10000141;
    //ll n = p * q;
    //ll f = (p - 1) * (q - 1) + 1;
    ll f, n;
    f = n = 10000229;
    ll res = 1;
    while(f > 0){
        if(f & 1)res = (res * m) % n;
        m = (m * m) % n;
        f >>= 1;
    }
    return res;
}