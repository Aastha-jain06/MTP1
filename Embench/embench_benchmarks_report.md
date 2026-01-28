# Embench IoT Benchmark Suite - Comprehensive Report

## Executive Summary

Embench is a free and open source benchmark suite designed to test the performance of deeply embedded systems, specifically targeting IoT-class devices. The suite comprises 19 real-world programs that replace outdated synthetic benchmarks like Dhrystone and CoreMark.

**Key Features:**
- All benchmarks fit within 64kB of program space and 64kB of RAM
- Each benchmark is scaled to run for approximately 4 seconds
- Uses geometric mean and standard deviation for performance scoring
- Licensed under GPL-3.0
- Released as version 1.0 in 2021 after pre-release 0.5 in February 2020

---

## Overview of the Benchmark Suite

### Purpose and Design Goals

Embench was explicitly designed to meet the requirements of modern connected embedded systems with relevant, portable, and well-implemented benchmarks. The project was founded by Turing Award winner Prof. David Patterson in late 2018 to address the inadequate benchmark methodology in the embedded computing community.

### Performance Measurement

- **Speed Measurement**: Uses platform's internal cycle counter registers
- **Size Measurement**: Total size of all .text sections in bytes
- **Scoring**: Relative performance compared to a baseline reference platform
- **Cache Behavior**: Designed to use "hot" caches with warm-up runs before timing

---

## The 19 Embench Benchmarks

### 1. **aha-mont64**
**Purpose**: Montgomery multiplication for modular arithmetic

**What it does**: 
Computes modular multiplication (a*b mod m) using Montgomery multiplication algorithm. This is a cryptographic operation used in public-key cryptography systems like RSA.

**Code characteristics**:
- Integer-intensive computation
- Tests 64-bit arithmetic operations
- Based on algorithms from "Hacker's Delight"

**Reference metrics**:
- Speed: ~4,004 cycles
- Size: 1,072 bytes

---

### 2. **crc32**
**Purpose**: Cyclic Redundancy Check calculation

**What it does**: 
Computes 32-bit CRC checksums for data integrity verification. CRC is widely used in network protocols, storage systems, and data transmission to detect errors in data.

**Code characteristics**:
- Memory-intensive operations
- Table lookups and bit manipulation
- Common in embedded communication protocols

**Reference metrics**:
- Speed: ~4,010 cycles
- Size: 284 bytes

---

### 3. **cubic**
**Purpose**: Cubic function solver

**What it does**: 
Solves cubic equations using numerical methods. This benchmark tests floating-point or fixed-point mathematical computation capabilities relevant to control systems and signal processing.

**Code characteristics**:
- Mathematical computation
- Iterative algorithms
- Tests arithmetic precision

**Reference metrics**:
- Speed: ~3,931 cycles
- Size: 1,584 bytes

---

### 4. **edn**
**Purpose**: Embedded data notation processing

**What it does**: 
Parses and processes structured data formats, similar to JSON or XML parsing which is common in IoT devices that need to exchange data with other systems.

**Code characteristics**:
- String processing
- Data structure manipulation
- Branch-intensive code

**Reference metrics**:
- Speed: ~4,010 cycles
- Size: 1,324 bytes

---

### 5. **huffbench**
**Purpose**: Huffman encoding/decoding

**What it does**: 
Implements Huffman compression algorithm for data compression. This is widely used in embedded systems for reducing storage requirements and transmission bandwidth.

**Code characteristics**:
- Tree-based data structures
- Bit manipulation
- Memory access patterns

**Reference metrics**:
- Speed: ~4,120 cycles
- Size: 1,242 bytes

---

### 6. **matmult-int**
**Purpose**: Integer matrix multiplication

**What it does**: 
Performs multiplication of integer matrices. Matrix operations are fundamental to many embedded applications including image processing, sensor fusion, and control algorithms.

**Code characteristics**:
- Nested loops
- Integer arithmetic
- Memory access patterns (cache performance)

**Reference metrics**:
- Speed: ~3,985 cycles
- Size: 492 bytes

---

### 7. **minver**
**Purpose**: Matrix inversion

**What it does**: 
Computes the inverse of a matrix, which is essential for solving systems of linear equations in control theory, robotics, and signal processing applications.

**Code characteristics**:
- Complex numerical algorithms
- Floating-point or fixed-point operations
- Tests numerical stability

**Reference metrics**:
- Speed: ~3,998 cycles
- Size: 1,168 bytes

---

### 8. **nbody**
**Purpose**: N-body physics simulation

**What it does**: 
Simulates gravitational interactions between multiple bodies. While seemingly academic, these algorithms are used in robotics path planning and collision detection.

**Code characteristics**:
- Floating-point intensive
- Computational loops
- Tests mathematical libraries

**Reference metrics**:
- Speed: ~2,808 cycles
- Size: 950 bytes

---

### 9. **nettle-aes**
**Purpose**: AES encryption

**What it does**: 
Implements the Advanced Encryption Standard (AES), the most widely used symmetric encryption algorithm. Critical for secure communication in IoT devices.

**Code characteristics**:
- Table lookups
- Bit operations
- Security-critical code patterns

**Reference metrics**:
- Speed: ~4,026 cycles
- Size: 2,148 bytes

---

### 10. **nettle-sha256**
**Purpose**: SHA-256 cryptographic hash

**What it does**: 
Computes SHA-256 cryptographic hash function used for data integrity, digital signatures, and password hashing in embedded security applications.

**Code characteristics**:
- Bitwise operations
- Fixed-size data blocks
- Deterministic computation

**Reference metrics**:
- Speed: ~3,997 cycles
- Size: 3,396 bytes

---

### 11. **nsichneu**
**Purpose**: Neural network simulation

**What it does**: 
Simulates a neural network, representing the increasing use of machine learning and AI algorithms in edge devices for pattern recognition and decision making.

**Code characteristics**:
- Floating-point operations
- Vector/matrix operations
- Memory-intensive

**Reference metrics**:
- Speed: ~4,001 cycles
- Size: 11,968 bytes (largest benchmark)

---

### 12. **picojpeg**
**Purpose**: JPEG image decompression

**What it does**: 
Decodes JPEG images, essential for embedded devices with cameras or image processing capabilities such as security cameras and industrial inspection systems.

**Code characteristics**:
- Table lookups
- Integer arithmetic
- Memory management
- DCT (Discrete Cosine Transform) operations

**Reference metrics**:
- Speed: ~4,030 cycles
- Size: 6,964 bytes

---

### 13. **qrduino**
**Purpose**: QR code generation

**What it does**: 
Generates QR codes for encoding data in 2D barcodes. Increasingly common in IoT devices for configuration, pairing, and data sharing.

**Code characteristics**:
- Algorithmic complexity
- Bit manipulation
- Array operations

**Reference metrics**:
- Speed: ~4,253 cycles
- Size: 5,814 bytes

---

### 14. **sglib-combined**
**Purpose**: Generic data structure library operations

**What it does**: 
Tests various common data structure operations (lists, sorting, searching) from the Simple Generic Library, representing typical embedded software data manipulation tasks.

**Code characteristics**:
- Pointer manipulation
- Memory allocation patterns
- Generic algorithm implementations

**Reference metrics**:
- Speed: ~3,981 cycles
- Size: 2,272 bytes

---

### 15. **slre**
**Purpose**: Regular expression matching

**What it does**: 
Implements a simple regular expression engine for pattern matching in strings. Used in text processing, parsing, and validation in embedded systems.

**Code characteristics**:
- String processing
- Branching logic
- State machine implementation

**Reference metrics**:
- Speed: ~4,010 cycles
- Size: 2,422 bytes (value from documentation)

---

### 16. **st**
**Purpose**: Statistical functions

**What it does**: 
Performs statistical calculations on data sets. Relevant for sensor data processing, quality control, and data analysis in embedded monitoring systems.

**Code characteristics**:
- Mathematical operations
- Array processing
- Accumulator patterns

**Reference metrics**:
- Speed: ~4,151 cycles (from BEEBS data)
- Size: 880 bytes

---

### 17. **statemate**
**Purpose**: State machine processing

**What it does**: 
Executes state machine logic, fundamental to embedded control systems, protocol handlers, and reactive systems that respond to events.

**Code characteristics**:
- Conditional logic
- State transitions
- Event handling

**Reference metrics**:
- Speed: ~4,000 cycles
- Size: 3,686 bytes

---

### 18. **ud**
**Purpose**: Lightweight Unicode processing

**What it does**: 
Performs Unicode character handling and conversion operations, important for internationalization and text display in embedded user interfaces.

**Code characteristics**:
- Character encoding/decoding
- Table lookups
- String operations

**Reference metrics**:
- Speed: ~4,001 cycles
- Size: 702 bytes

---

### 19. **wikisort**
**Purpose**: Sorting algorithm

**What it does**: 
Implements an optimized sorting algorithm (WikiSort is a block merge sort). Sorting is a fundamental operation in data processing, searching, and organization.

**Code characteristics**:
- Comparison operations
- Memory swapping
- Cache-aware algorithms

**Reference metrics**:
- Speed: ~4,226 cycles
- Size: 4,208 bytes

---

## Benchmark Categories

The 19 benchmarks can be grouped into functional categories:

### **Cryptography & Security** (3)
- nettle-aes (AES encryption)
- nettle-sha256 (SHA-256 hashing)
- aha-mont64 (Montgomery multiplication for RSA)

### **Data Compression & Encoding** (2)
- huffbench (Huffman compression)
- picojpeg (JPEG decompression)

### **Mathematical Computation** (5)
- cubic (Equation solving)
- matmult-int (Matrix multiplication)
- minver (Matrix inversion)
- nbody (Physics simulation)
- st (Statistical functions)

### **Data Structures & Algorithms** (4)
- sglib-combined (Generic library operations)
- wikisort (Sorting)
- slre (Regular expressions)
- edn (Data notation parsing)

### **Embedded-Specific Applications** (3)
- qrduino (QR code generation)
- statemate (State machines)
- nsichneu (Neural networks)

### **Low-Level Operations** (2)
- crc32 (Checksum calculation)
- ud (Unicode processing)

---

## Technical Specifications

### System Requirements
- **Program Space**: All benchmarks fit in 64kB
- **RAM Usage**: Maximum 64kB RAM required
- **OS Requirements**: No OS required, minimal C library support
- **Output**: No output stream dependencies

### Build System
- Python scripts for building and running (Python 3.6+)
- Out-of-tree build system
- Cross-compilation support for multiple architectures
- Configuration files for different CPUs and boards

### Supported Architectures
The benchmark suite includes configurations for:
- ARM (Cortex-M series)
- RISC-V (RV32, RV64)
- Native x86/x64 (for development)
- And others through the extensible configuration system

---

## Performance Scoring Methodology

### How Benchmarks are Scored

1. **Individual Benchmark Score**: Each benchmark runs on the target platform and the execution time is measured
2. **Normalization**: Times are normalized by CPU frequency (MHz) to account for different clock speeds
3. **Relative Performance**: Results are compared against baseline reference platform measurements
4. **Geometric Mean**: Final score is the geometric mean of all relative performance ratios
5. **Geometric Standard Deviation**: Variability metric across benchmarks

### Why Geometric Mean?

Geometric mean is recommended because it properly handles ratios and provides a single summarizing performance score that represents overall platform performance while being less sensitive to outliers than arithmetic mean.

---

## Usage and Applications

### Industry Adoption

Seagate Technology has been using Embench benchmarks to measure the performance of their RISC-V cores, demonstrating real-world industrial application.

### Academic Use

Rice University helped develop the DSP branch of Embench and uses it in their Vertically Integrated Projects course, showing its educational value.

### Typical Use Cases

1. **Compiler Optimization**: Testing different compiler flags and optimization levels
2. **Hardware Comparison**: Comparing different microcontroller architectures
3. **Tool Chain Evaluation**: Assessing the quality of development tools
4. **Design Trade-offs**: Understanding performance vs. code size trade-offs
5. **Regression Testing**: Ensuring performance doesn't degrade across software versions

---

## Future Development

The Embench project continues to evolve:

- **Two-Year Update Cycle**: New major releases planned every two years to maintain relevance
- **Floating-Point Suite**: Development of specialized FP benchmarks for DSP applications
- **Application-Class Suite**: Future suite for larger embedded systems with operating systems
- **Community Contributions**: Open collaboration between industry and academia

---

## Comparison with Other Benchmarks

### Why Not Dhrystone or CoreMark?

Dhrystone (circa 1984) and CoreMark (circa 2009) are synthetic programs that cannot predict real program performance. Embench addresses this by using actual application code.

**Advantages of Embench**:
- Real-world application code instead of synthetic loops
- Multiple diverse benchmarks vs. single test
- Modern workloads relevant to IoT devices
- Open source and freely modifiable
- Statistically robust scoring methodology
- Active community and regular updates

---

## Conclusion

The Embench IoT benchmark suite represents a significant advancement in embedded systems performance evaluation. By providing 19 diverse, real-world benchmarks that test different aspects of embedded system performance—from cryptography and compression to mathematical computation and data processing—Embench offers a comprehensive and meaningful way to compare embedded platforms.

The suite's focus on actual application code, its carefully designed methodology, and its open-source nature make it suitable for both industrial and academic use. As IoT and embedded systems continue to grow in importance, Embench provides the tools needed to make informed decisions about hardware selection, compiler optimization, and system design.

---

## References and Resources

- **Main Website**: https://www.embench.org/
- **GitHub Repository**: https://github.com/embench/embench-iot
- **Documentation**: Available in the repository's doc/ directory
- **Mailing List**: Available through embench.org for community discussions
- **License**: GNU General Public License version 3 (GPL-3.0)

---

*Report generated based on Embench IoT version 1.0*
*Last updated: January 2026*
