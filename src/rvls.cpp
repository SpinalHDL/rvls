#include "ascii_frontend.hpp"


using namespace std;


int main(){
    cout << "miaou3" << endl;
    std::ifstream f("../../trace.txt");
    checkFile(f);
}

