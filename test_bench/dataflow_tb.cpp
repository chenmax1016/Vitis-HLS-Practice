#include <iostream>
#include "../src/dataflow.h"

int main(void){
    int a[16];
    int result[16];

    for(int i = 0; i < 16; i++){
        a[i] = i;
    }

    dataflow(a, result);

    for(int i = 0; i < 16; i++){
        if(result[i] != a[i] * 2){
            std::cout << "FAIL!" << std::endl;
            return 1;
        }
    }

    std::cout << "PASS!" << std::endl;





    return 0;
}