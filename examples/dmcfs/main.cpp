#include "dmcfs.h"

int main(int argc, char* argv[]) {
    Idmcfs* module = dmcfsGetModule();
    if (module) {
        module->Release();
    }
    return 0;
}