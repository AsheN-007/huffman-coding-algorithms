# Data Compression: Static Huffman vs. Adaptive Huffman (FGK)

A C++ implementation of two classic lossless data compression algorithms based on variable-length prefix codes:

1. **Static Huffman Coding (`huffman.cc`):** A two-pass algorithm that computes global character frequencies across the input file, builds an optimal Huffman tree, and serializes the frequency table into the compressed file header.
2. **Adaptive Huffman Coding / FGK Algorithm (`fgk.cc`):** A single-pass implementation of the Faller–Gallager–Knuth algorithm. It dynamically builds and updates the Huffman tree on the fly using NYT (*Not Yet Transmitted*) nodes, eliminating the need to store a frequency table in the output file.

## Project Structure

* `Sample Texts` — Sample files meant to be used for testing the compression.
* `huffman.cc` — Static Huffman implementation with byte-level bit packing (`out.bin` / `decoded.txt`).
* `fgk.cc` — Adaptive FGK Huffman implementation with dynamic tree updates and bit-level I/O (`out.bin` / `decoded.txt`).
* `Study.pdf` — A study on the comparisson of both algorithms by the authors to FIB-UPC.
* `README.md` — Project documentation and usage guide.


## Compilation

Compiling both programs with optimization enabled (`-O3`) is recommended for accurate timing benchmarks on larger files:

```bash
g++ -std=c++17 -O3 huffman.cc -o huffman
g++ -std=c++17 -O3 fgk.cc -o fgk
```

## Usage

Both executables share a consistent command-line interface:

```bash
./huffman {-e | -d} [-t] <file>
./fgk     {-e | -d} [-t] <file>
```

### Options

| Flag | Description |
| :--- | :--- |
| `-e` | **Encode (Compress):** Reads the input file and generates the compressed binary file `out.bin`. |
| `-d` | **Decode (Decompress):** Reads a compressed file and reconstructs the original file as `decoded.txt`. |
| `-t` | **Timer:** Measures and prints the elapsed execution time for the encoding or decoding process. This flag is optional.|

## Examples

### 1. Static Huffman (`huffman`)

**Compress a text file while measuring execution time:**
```bash
./huffman -e -t Sample\ Texts/alice29.txt
```
*(Outputs `out.bin` and prints the elapsed encoding time).*

**Decompress the generated file:**
```bash
./huffman -d -t out.bin
```
*(Outputs `decoded.txt`).*

### 2. Adaptive FGK Huffman (`fgk`)

**Compress the same file with FGK:**
```bash
./fgk -e -t Sample\ Texts/alice29.txt
```
*(Outputs `out.bin` in a single pass with no frequency header).*

**Decompress with FGK:**
```bash
./fgk -d -t out.bin
```
*(Outputs `decoded.txt`).*

### 3. Verify Lossless Integrity

Confirm that the decompressed output matches the original file byte-for-byte using `diff`:

```bash
diff -s Sample\ Texts/alice29.txt decoded.txt
```
