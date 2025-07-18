
#include "dmcfs.h"
#include "gtest.h"

class env_dmcfs
{
public:
    void init(){}
    void uninit(){}
};

class frame_dmcfs : public testing::Test
{
public:
    virtual void SetUp()
    {
        env.init();
    }
    virtual void TearDown()
    {
        env.uninit();
    }
protected:
    env_dmcfs env;
};

TEST_F(frame_dmcfs, init)
{
    Idmcfs* module = dmcfsGetModule();
    if (module)
    {
        module->Test();
        module->Release();
    }
}
