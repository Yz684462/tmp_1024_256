
    def _modify_translated_code(self, translated: str, func_name: str) -> str:
        """Modify translated code to handle simulated_cpu_state with thread-specific offsets."""
        lines = translated.splitlines()
        modified_lines = []
        
        # Remove global simulated_cpu_state declarations
        for line in lines:
            if '.globl simulated_cpu_state' in line:
                continue
            if '.comm   simulated_cpu_state' in line:
                continue
            modified_lines.append(line)
            if '.type' in line:
                modified_lines.append('\t.extern global_simulated_vector_contexts_pool')
        
        translated = '\n'.join(modified_lines)
        
        # First replace simulated_cpu_state with global_simulated_vector_contexts_pool
        translated = translated.replace('simulated_cpu_state', 'global_simulated_vector_contexts_pool')
        
        # Add thread-specific offset after each "la t6, simulated_cpu_state"
        offset = self.thread_index * VECTOR_CONTEXT_SIZE
        
        # Then add offset after each "la t6, global_simulated_vector_contexts_pool"
        pattern = r'(\s+la\s+t6,\s+global_simulated_vector_contexts_pool)'
        
        # Use multiple instructions if offset is too large for addi
        if offset <= 2047:
            # Single addi instruction is sufficient
            replacement = f'\\1\n\taddi\tt6, t6, {offset}'
        else:
            # Need to use li + add sequence for large offsets
            replacement = f'\\1\n\tli\tt0, {offset}\n\tadd\tt6, t6, t0'
        
        translated = re.sub(pattern, replacement, translated)
        
        return translated