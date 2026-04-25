#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "../include/utils.h"

namespace fs = std::filesystem;

class DumpAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {    
        // Initialize DumpAnalyzer with test file
        analyzer = &BinaryTranslation::Dump::DumpAnalyzer::getInstance("../../../dump.txt");
    }
    
    void TearDown() override {
        // Clean up test file
        analyzer = nullptr;
    }
    
    BinaryTranslation::Dump::DumpAnalyzer* analyzer;
};

// Test parse_instr_line function
TEST_F(DumpAnalyzerTest, ParseInstrLine) {
    // Test valid instruction line
    std::string valid_line = "  1079cc: 6442                 ld      s0,16(sp)";
    BinaryTranslation::Instruction* instr = analyzer->parse_instr_line(valid_line);
    
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->address, 0x1079cc);
    EXPECT_EQ(instr->opcode, "ld");
    EXPECT_EQ(instr->operands, std::vector<std::string>({"s0", "16(sp)"}));
    EXPECT_EQ(instr->instrlen, 2);
    
    // Test invalid line
    std::string invalid_line = "invalid line";
    BinaryTranslation::Instruction* instr2 = analyzer->parse_instr_line(invalid_line);
    EXPECT_EQ(instr2, nullptr);
}

// Test parse_func_line function
TEST_F(DumpAnalyzerTest, ParseFuncLine) {
    // Test function line parsing
    std::string func_line = "00000000001079d2 <ggml_set_op_params_i32>:";
    analyzer->parse_func_line(func_line);
    
    // Check if function address was recorded
    EXPECT_TRUE(analyzer->parsed_func_addrs_.find(0x1079d2) != analyzer->parsed_func_addrs_.end());
    
    // Test non-function line
    std::string non_func_line = "  1079cc: 6442                 ld      s0,16(sp)";
    size_t initial_size = analyzer->parsed_func_addrs_.size();
    analyzer->parse_func_line(non_func_line);
    EXPECT_EQ(analyzer->parsed_func_addrs_.size(), initial_size);
}

// Test parse_line_at_addr function
TEST_F(DumpAnalyzerTest, ParseLineAtAddr) {
    // Test finding existing instruction
    BinaryTranslation::Instruction* instr = analyzer->parse_line_at_addr(0x1079cc);
    ASSERT_NE(instr, nullptr);
    EXPECT_EQ(instr->address, 0x1079cc);
    EXPECT_EQ(instr->opcode, "ld");
    
    // Test finding non-existing instruction
    BinaryTranslation::Instruction* instr2 = analyzer->parse_line_at_addr(0x999999);
    EXPECT_EQ(instr2, nullptr);
    
    // Test address beyond range
    BinaryTranslation::Instruction* instr3 = analyzer->parse_line_at_addr(0x108000);
    EXPECT_EQ(instr3, nullptr);
}

// Test select_snippet function
TEST_F(DumpAnalyzerTest, SelectSnippet) {
    std::pair<uint64_t, uint64_t> range = {0x1079cc, 0x1079d0};
    std::vector<BinaryTranslation::Instruction*> snippet = analyzer->select_snippet(range);
    
    EXPECT_EQ(snippet.size(), 2);  // Should find 4 instructions
    
    // Verify first instruction
    EXPECT_EQ(snippet[0]->address, 0x1079cc);
    EXPECT_EQ(snippet[0]->opcode, "ld");
    
    // Verify last instruction
    EXPECT_EQ(snippet[1]->address, 0x1079ce);
    EXPECT_EQ(snippet[1]->opcode, "addi");
    
}

// Test select_func_content function
TEST_F(DumpAnalyzerTest, SelectFuncContent) {
    std::vector<BinaryTranslation::Instruction*> func_content = analyzer->select_func_content(0x1079d4);

    EXPECT_EQ(func_content.size(), 6);  // Should find some instructions
    
    // Verify instructions are within function range
    for (const auto& instr : func_content) {
        EXPECT_GE(instr->address, 0x1079d2);
        EXPECT_LT(instr->address, 0x1079de);
    }
}
