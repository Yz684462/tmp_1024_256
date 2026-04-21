#include <gtest/gtest.h>
#include "../include/utils.h"

class CodeBlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test instructions
        test_instructions.clear();
        
        // Create some test instructions
        BinaryTranslation::Instruction* instr1 = new BinaryTranslation::Instruction("add", "a0, a1", 0x1000, 4);
        BinaryTranslation::Instruction* instr2 = new BinaryTranslation::Instruction("sub", "a1, a2", 0x1004, 4);
        BinaryTranslation::Instruction* instr3 = new BinaryTranslation::Instruction("beq", "a0, a1", 0x1008, 4);
        BinaryTranslation::Instruction* instr4 = new BinaryTranslation::Instruction("jalr", "ra", 0x100c, 4);
        
        test_instructions.push_back(instr1);
        test_instructions.push_back(instr2);
        test_instructions.push_back(instr3);
        test_instructions.push_back(instr4);
    }
    
    void TearDown() override {
        // Clean up instructions
        for (auto* instr : test_instructions) {
            delete instr;
        }
        test_instructions.clear();
    }
    
    std::vector<BinaryTranslation::Instruction*> test_instructions;

public:
    // Test get_codeblocks_linear function
    TEST_F(CodeBlockTest, GetCodeblocksLinear) {
        std::vector<BinaryTranslation::CodeBlock*> codeblocks = 
            BinaryTranslation::CodeBlock::get_codeblocks_linear(test_instructions);
        
        // Should return one codeblock containing all instructions
        EXPECT_EQ(codeblocks.size(), 1);
        
        if (!codeblocks.empty()) {
            BinaryTranslation::CodeBlock* block = codeblocks[0];
            EXPECT_EQ(block->instructions.size(), 4);
            
            // Verify first instruction
            EXPECT_EQ(block->instructions[0]->opcode, "add");
            EXPECT_EQ(block->instructions[0]->address, 0x1000);
            
            // Verify last instruction
            EXPECT_EQ(block->instructions[3]->opcode, "jalr");
            EXPECT_EQ(block->instructions[3]->address, 0x100c);
        }
    }
    
    // Test with empty instructions
    TEST_F(CodeBlockTest, EmptyInstructions) {
        std::vector<BinaryTranslation::CodeBlock*> empty_blocks = 
            BinaryTranslation::CodeBlock::get_codeblocks_linear({});
        
        EXPECT_TRUE(empty_blocks.empty());
    }
    
    // Test with single instruction
    TEST_F(CodeBlockTest, SingleInstruction) {
        std::vector<BinaryTranslation::Instruction*> single_instr;
        single_instr.push_back(new BinaryTranslation::Instruction("nop", "", 0x2000, 4));
        
        std::vector<BinaryTranslation::CodeBlock*> single_blocks = 
            BinaryTranslation::CodeBlock::get_codeblocks_linear(single_instr);
        
        EXPECT_EQ(single_blocks.size(), 1);
        EXPECT_EQ(single_blocks[0]->instructions.size(), 1);
        EXPECT_EQ(single_blocks[0]->instructions[0]->opcode, "nop");
        
        delete single_instr[0];
    }
};
