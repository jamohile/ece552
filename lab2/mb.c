#include <stdio.h>
int a;
for (int i =0; i<100000; i++) {
    for (int j=0; j<7;j++) {
        if (i % 7 ==0) {
            a = 15;
        }
        else {
            a =10;
        }
    }
}