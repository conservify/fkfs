#include <gtest/gtest.h>

class BasicSuite : public ::testing::Test {
protected:
    BasicSuite();
    virtual ~BasicSuite();

    virtual void SetUp();
    virtual void TearDown();

};
