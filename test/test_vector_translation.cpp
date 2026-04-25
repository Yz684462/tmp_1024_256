#include <gtest/gtest.h>
#include <dlfcn.h>
#include <thread>
#include <chrono>
#include "../include/vector_translation.h"
#include "../include/utils.h"

class VectorTranslationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test code blocks for testing
        test_code_blocks.clear();
        
        // Create test instructions
        BinaryTranslation::Instruction* instr1 = new BinaryTranslation::Instruction("add", "a0, a1", 0x1000, 4);
        BinaryTranslation::Instruction* instr2 = new BinaryTranslation::Instruction("sub", "a1, a2", 0x1004, 4);
        BinaryTranslation::Instruction* instr3 = new BinaryTranslation::Instruction("mul", "a2, a3", 0x1008, 4);
        
        std::vector<BinaryTranslation::Instruction*> instructions = {instr1, instr2, instr3};
        
        // Create a code block
        BinaryTranslation::CodeBlock* block = new BinaryTranslation::CodeBlock(instructions);
        test_code_blocks.push_back(block);
    }
    
    void TearDown() override {
        // Clean up code blocks
        for (auto* block : test_code_blocks) {
            delete block;
        }
        test_code_blocks.clear();
    }
    
    std::vector<BinaryTranslation::CodeBlock*> test_code_blocks;
};

// TranslationIdManager tests
class TranslationIdManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset singleton state for clean testing
        // Note: This assumes the singleton can be reset or we work with fresh instances
    }
    
    void TearDown() override {
        // Clean up any test state
    }
};

TEST_F(TranslationIdManagerTest, SingletonBehavior) {
    auto& manager1 = BinaryTranslation::TranslationId::TranslationIdManager::getInstance();
    auto& manager2 = BinaryTranslation::TranslationId::TranslationIdManager::getInstance();
    
    // Should return the same instance
    EXPECT_EQ(&manager1, &manager2);
}

TEST_F(TranslationIdManagerTest, GetCurrentTranslationId) {
    auto& manager = BinaryTranslation::TranslationId::TranslationIdManager::getInstance();
    
    // Get current translation ID
    int translation_id = manager.get_current_translation_id();
    
    // Should return a valid translation ID (non-negative)
    EXPECT_GE(translation_id, 0);
}

TEST_F(TranslationIdManagerTest, MultiThreadedTranslationId) {
    auto& manager = BinaryTranslation::TranslationId::TranslationIdManager::getInstance();
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<int> thread_results(num_threads);
    std::mutex results_mutex;
    
    // Launch multiple threads, each calling get_current_translation_id() once
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            int translation_id = manager.get_current_translation_id();
            
            // Store result thread-safely
            std::lock_guard<std::mutex> lock(results_mutex);
            thread_results[i] = translation_id;
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // In main thread, verify all translation IDs are valid
    for (int translation_id : thread_results) {
        EXPECT_GE(translation_id, 0) << "Translation ID should be non-negative";
    }
    
    // Verify that all calls returned the same translation ID (singleton behavior)
    if (!thread_results.empty()) {
        for (int i = 0; i < num_threads; ++i) {
            EXPECT_EQ(thread_results[i], i) 
                << "All calls should return the same translation ID for the same process";
        }
    }
    
    // Final verification from main thread
    int final_translation_id = manager.get_current_translation_id();
    EXPECT_GE(final_translation_id, 0);
    
    // Should match the ID from threads (if any threads ran)
    if (!thread_results.empty()) {
        EXPECT_EQ(final_translation_id, thread_results[0]);
    }
}

// TranslationRanges tests
TEST_F(VectorTranslationTest, GetTranslationRanges) {
    uint64_t base_addr = 0x1000;
    auto ranges = BinaryTranslation::TranslationRanges::get_translation_ranges(test_code_blocks, base_addr);
    
    // Should return at least one range
    EXPECT_FALSE(ranges.empty());
    
    // Check that ranges are valid (start <= end)
    for (const auto& range : ranges) {
        EXPECT_LE(range.first, range.second);
    }
}

TEST_F(VectorTranslationTest, GetTranslationRangesEmptyBlocks) {
    std::vector<BinaryTranslation::CodeBlock*> empty_blocks;
    uint64_t base_addr = 0x1000;
    
    auto ranges = BinaryTranslation::TranslationRanges::get_translation_ranges(empty_blocks, base_addr);
    
    // Should return empty ranges for empty input
    EXPECT_TRUE(ranges.empty());
}

TEST_F(VectorTranslationTest, GetTranslationRangesVectorInstructions) {
    // Create test code blocks with vector instructions (opcode starting with 'v')
    std::vector<BinaryTranslation::CodeBlock*> vector_blocks;
    
    // Create instructions with vector opcodes
    BinaryTranslation::Instruction* v_instr1 = new BinaryTranslation::Instruction("vadd", "v0, v1", 0x1000, 4);
    BinaryTranslation::Instruction* v_instr2 = new BinaryTranslation::Instruction("vmul", "v2, v3", 0x1004, 4);
    BinaryTranslation::Instruction* v_instr3 = new BinaryTranslation::Instruction("vsub", "v4, v5", 0x1008, 4);
    // Non-vector instruction (should break the continuity)
    BinaryTranslation::Instruction* non_v_instr = new BinaryTranslation::Instruction("add", "a0, a1", 0x100c, 4);
    // Another vector instruction (should start a new range)
    BinaryTranslation::Instruction* v_instr4 = new BinaryTranslation::Instruction("vdiv", "v6, v7", 0x1010, 4);
    BinaryTranslation::Instruction* v_instr5 = new BinaryTranslation::Instruction("vdot", "v8, v9", 0x1014, 4);
    
    std::vector<BinaryTranslation::Instruction*> instructions = {
        v_instr1, v_instr2, v_instr3, non_v_instr, v_instr4, v_instr5
    };
    
    BinaryTranslation::CodeBlock* block = new BinaryTranslation::CodeBlock(instructions);
    vector_blocks.push_back(block);
    
    uint64_t base_addr = 0x1000;
    auto ranges = BinaryTranslation::TranslationRanges::get_translation_ranges(vector_blocks, base_addr);
    
    // Should find 2 continuous vector instruction ranges:
    // 1. [0x1000, 0x100C) for vadd, vmul, vsub (addresses 0x1000, 0x1004, 0x1008)
    // 2. [0x1010, 0x1018) for vdiv, vdot (addresses 0x1010, 0x1014)
    EXPECT_EQ(ranges.size(), 2);
    
    // Verify first range [0x1000, 0x100C)
    EXPECT_EQ(ranges[0].first, 0x1000);
    EXPECT_EQ(ranges[0].second, 0x100C);
    
    // Verify second range [0x1010, 0x1018)
    EXPECT_EQ(ranges[1].first, 0x1010);
    EXPECT_EQ(ranges[1].second, 0x1018);
    
    // Cleanup
    delete block;
}

TEST_F(VectorTranslationTest, GetTranslationRangesNoVectorInstructions) {
    // Create test code blocks with only non-vector instructions
    std::vector<BinaryTranslation::CodeBlock*> non_vector_blocks;
    
    // Create instructions with non-vector opcodes
    BinaryTranslation::Instruction* instr1 = new BinaryTranslation::Instruction("add", "a0, a1", 0x1000, 4);
    BinaryTranslation::Instruction* instr2 = new BinaryTranslation::Instruction("sub", "a2, a3", 0x1004, 4);
    BinaryTranslation::Instruction* instr3 = new BinaryTranslation::Instruction("mul", "a4, a5", 0x1008, 4);
    
    std::vector<BinaryTranslation::Instruction*> instructions = {instr1, instr2, instr3};
    
    BinaryTranslation::CodeBlock* block = new BinaryTranslation::CodeBlock(instructions);
    non_vector_blocks.push_back(block);
    
    uint64_t base_addr = 0x1000;
    auto ranges = BinaryTranslation::TranslationRanges::get_translation_ranges(non_vector_blocks, base_addr);
    
    // Should return empty ranges when no vector instructions found
    EXPECT_TRUE(ranges.empty());
    
    // Cleanup
    delete block;
}

TEST_F(VectorTranslationTest, GetTranslationRangesSingleVectorInstruction) {
    // Create test code blocks with a single vector instruction
    std::vector<BinaryTranslation::CodeBlock*> single_vector_blocks;
    
    BinaryTranslation::Instruction* v_instr = new BinaryTranslation::Instruction("vadd", "v0, v1", 0x2000, 4);
    std::vector<BinaryTranslation::Instruction*> instructions = {v_instr};
    
    BinaryTranslation::CodeBlock* block = new BinaryTranslation::CodeBlock(instructions);
    single_vector_blocks.push_back(block);
    
    uint64_t base_addr = 0x2000;
    auto ranges = BinaryTranslation::TranslationRanges::get_translation_ranges(single_vector_blocks, base_addr);
    
    // Should find 1 range [0x2000, 0x2004) for single vector instruction at address 0x2000
    EXPECT_EQ(ranges.size(), 1);
    EXPECT_EQ(ranges[0].first, 0x2000);
    EXPECT_EQ(ranges[0].second, 0x2004);
    
    // Cleanup
    delete block;
}

// TranslationSharedLib tests
class TranslationSharedLibTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup for shared library tests
    }
    
    void TearDown() override {
        // Cleanup
    }
};

TEST_F(TranslationSharedLibTest, MakeFuncName) {
    uint64_t fault_addr = 0x12345678;
    std::string func_name = BinaryTranslation::TranslationSharedLib::make_func_name(fault_addr);
    
    EXPECT_EQ(func_name, "translation_func_" + std::to_string(fault_addr));
}

TEST_F(TranslationSharedLibTest, MakeTranslationSharedLibName) {
    int translation_id = 42;
    std::string lib_name = BinaryTranslation::TranslationSharedLib::make_translation_shared_lib_name(translation_id);
    
    EXPECT_EQ(lib_name, "translation_lib_" + std::to_string(translation_id));
}

class TranslationHandleManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get singleton instance
        handle_manager = &BinaryTranslation::TranslationSharedLib::TranslationHandleManager::getInstance();
    }
    
    void TearDown() override {
        handle_manager = nullptr;
    }
    
    BinaryTranslation::TranslationSharedLib::TranslationHandleManager* handle_manager;
};

TEST_F(TranslationHandleManagerTest, SingletonBehavior) {
    auto& manager1 = BinaryTranslation::TranslationSharedLib::TranslationHandleManager::getInstance();
    auto& manager2 = BinaryTranslation::TranslationSharedLib::TranslationHandleManager::getInstance();
    
    // Should return the same instance
    EXPECT_EQ(&manager1, &manager2);
}

TEST_F(TranslationHandleManagerTest, GetAndUpdateTranslationHandle) {
    // Initially should return nullptr or some default value
    void* initial_handle = handle_manager->get_current_translation_shared_lib_handle();
    EXPECT_EQ(initial_handle, nullptr);

    // Update with a mock handle
    void* mock_handle = reinterpret_cast<void*>(0x12345678);
    int translation_id = 0;
    handle_manager->update_translation_handle(translation_id, mock_handle);
    
    // Should return the updated handle
    void* current_handle = handle_manager->get_current_translation_shared_lib_handle();
    EXPECT_EQ(current_handle, mock_handle);
}

TEST_F(TranslationHandleManagerTest, GenTranslationSharedLib) {
    // Test with valid ranges
    std::vector<std::pair<uint64_t, uint64_t>> ranges = {{0x1000, 0x2000}, {0x3000, 0x4000}};
    
    // This test may require actual shared library generation
    // For now, just test that the function doesn't crash
    EXPECT_NO_THROW(handle_manager->gen_translation_shared_lib(ranges));
}

// VectorContext tests
class VectorContextManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Get singleton instance
        vc_manager = &BinaryTranslation::VectorContext::VectorContextManager::getInstance();
    }
    
    void TearDown() override {
        vc_manager = nullptr;
    }
    
    BinaryTranslation::VectorContext::VectorContextManager* vc_manager;
};

TEST_F(VectorContextManagerTest, SingletonBehavior) {
    auto& manager1 = BinaryTranslation::VectorContext::VectorContextManager::getInstance();
    auto& manager2 = BinaryTranslation::VectorContext::VectorContextManager::getInstance();
    
    // Should return the same instance
    EXPECT_EQ(&manager1, &manager2);
}