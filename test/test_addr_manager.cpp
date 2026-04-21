#include <gtest/gtest.h>
#include "../include/utils.h"

class AddrManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test with base address 0x1000
        manager = &BinaryTranslation::Addr::AddrManager::getInstance(0x1000);
    }
    
    void TearDown() override {
        manager = nullptr;
    }
    
    BinaryTranslation::Addr::AddrManager* manager;

public:
    // Test address conversion functions
    TEST_F(AddrManagerTest, ToAbs) {
        uint64_t rela_addr = 0x100;
        uint64_t abs_addr = manager->to_abs(rela_addr);
        EXPECT_EQ(abs_addr, 0x1100);  // 0x1000 + 0x100 = 0x1100
    }
    
    TEST_F(AddrManagerTest, ToRela) {
        uint64_t abs_addr = 0x1100;
        uint64_t rela_addr = manager->to_rela(abs_addr);
        EXPECT_EQ(rela_addr, 0x100);  // 0x1100 - 0x1000 = 0x100
    }
    
    // Test singleton behavior
    TEST_F(AddrManagerTest, Singleton) {
        // Get instance with different base address
        BinaryTranslation::Addr::AddrManager& manager2 = BinaryTranslation::Addr::AddrManager::getInstance(0x2000);
        
        // Should return same instance (ignoring new base address after first call)
        EXPECT_EQ(&manager2, manager);
    }
};
