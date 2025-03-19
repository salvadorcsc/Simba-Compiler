# Simba Compiler

This project was created in the summer of 2024 by me (finishing 3rd year of Computer Science and Engineering at the time) as a way to learn, explore and deepen my knowledge about compilers, programming languages and memory management.

In the end, I feel like it helped me get a better overall view of everything that is happening while we write code. I also learned a lot about memory management and understood why some simple changes in code can make major differences in terms of efficiency.

## Getting Started

### Prerequisites

To build and run the Simba compiler toolchain, you'll need:

- **C++ Compiler** (g++)
- **NASM** (Netwide Assembler)
- **ld** (GNU Linker)
- **Bash** or compatible shell

### Building the Project

1. Clone the repository:
   ```bash
   git clone https://github.com/yourusername/simba-compiler.git
   cd simba-compiler
   ```

2. Build the compiler components:
   ```bash
   make
   ```
   This will compile the three main components: `simba.o`, `sir.o`, and `sasm.o` in the `build` directory.

3. Make all shell scripts executable:
   ```bash
   chmod +x *.sh
   chmod +x build/scripts/*.sh
   ```
4. (Optional) Create an alias to use compiler:
    ```bash
    alias simba="./compile.sh"
    ```
    note: you can also run the compiler with "./compile.sh"

### Compiling a Simba Program

To compile and run a Simba program:

```bash
simba example.simba
```

This will:
1. Convert Simba (`.simba`) to SimbaIR (`.sir`)
2. Convert SimbaIR to SASM (`.sasm`)
3. Assemble SASM to machine code
4. Execute the resulting binary

note: all the files will be created in the same directory as the first one

### Cleanup Option

If you want to remove intermediate files after compilation:

```bash
simba -nv example.simba
```

The `-nv` (no verbose) flag will clean up all generated intermediate files after execution.

### Compilation Pipeline

The compilation process follows these steps:
- Simba (`.simba`) → SimbaIR (`.sir`) → SASM (`.sasm`) → Assembly (`.asm`) → Executable

Each step can also be run individually using the scripts in the `build/scripts` directory.
