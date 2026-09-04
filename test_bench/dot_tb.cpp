#include <iostream>
#include "../src/dot.h"

int main(void){

    int a[N];
    int b[N];
    
    int result; 

    for(int i = 0; i < N; i++){
        a[i] = i;
        b[i] = 2;
    }

    dot(a, b, &result);

    int golden = 0;
    for(int i = 0; i < N; i++){
        golden += a[i] * b[i];
    }

    std::cout << "HLS result = " << result << std::endl;
    std::cout << "GOLDEN     = " << golden << std::endl;

    if(result == golden){
        std::cout << "PASS!" << std::endl;
        return 0;
    }else{
        std::cout << "FAIL!" << std::endl;
        return 1;
    }


}
