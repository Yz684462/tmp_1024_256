#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include "../include/vector_translation.h"
#include "../include/utils.h"

class MakeDumpFragmentsFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize DumpAnalyzer with ../dump.s file
        auto& dump_analyzer = BinaryTranslation::Dump::DumpAnalyzer::getInstance("../../../dump.s");
    }
    
    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all(test_dir);
    }
    
    std::string test_dir;
};

TEST_F(MakeDumpFragmentsFileTest, BasicFunctionality) {
    // Get TranslationHandleManager instance
    auto& handle_manager = BinaryTranslation::TranslationSharedLib::TranslationHandleManager::getInstance();
    
    // Test ranges for vector instructions
    std::vector<std::pair<uint64_t, uint64_t>> ranges = {
        {0x2107a, 0x21082},  // Should extract vadd and vmul
        {0x21086, 0x2108e}   // Should extract vdiv and vdot
    };
    
    std::string output_file = "../test/output_fragments.s";
    
    // Call the function under test
    EXPECT_NO_THROW(handle_manager.make_dump_fragments_file(ranges, output_file));
}