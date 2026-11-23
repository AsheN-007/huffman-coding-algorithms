#include <iostream>
#include <fstream>
#include <queue>
#include <map>
#include <vector>
#include <string>
#include <cstdint>

using namespace std;

struct Node {
    char ch;
    int freq;
    Node *left, *right;
    Node(char ch, int freq, Node* left = nullptr, Node* right = nullptr)
        : ch(ch), freq(freq), left(left), right(right) {}
};

struct compare {
    bool operator()(Node* l, Node* r) {
        if (l->freq != r->freq) {
            return l->freq > r->freq; // Primary: lowest frequency first
        }
        return l->ch > r->ch;         // Tie-breaker: deterministic ASCII order
    }
}; 

// Shared helper: builds the Huffman tree from any frequency map
Node* buildTreeFromFreq(const map<char, int>& freq) {
    priority_queue<Node*, vector<Node*>, compare> pq;

    for (auto pair : freq) {
        pq.push(new Node(pair.first, pair.second));
    }

    // Handle single-character input edge case
    if (pq.size() == 1) {
        Node* only = pq.top();
        pq.pop();
        return new Node('\0', only->freq, only, nullptr);
    }

    while (pq.size() > 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();
        pq.push(new Node('\0', left->freq + right->freq, left, right));
    }

    return pq.empty() ? nullptr : pq.top();
}

void generateCodes(Node* root, string str, map<char, string>& huffmanCode) {
    if (!root) return;
    if (!root->left && !root->right) {
        huffmanCode[root->ch] = str;
    }
    generateCodes(root->left, str + "0", huffmanCode);
    generateCodes(root->right, str + "1", huffmanCode);
}

void encodeFile(ifstream& inFile) {
    map<char, int> freq;
    char ch;
    uint64_t totalChars = 0;

    while (inFile.get(ch)) {
        freq[ch]++;
        totalChars++;
    }

    Node* root = buildTreeFromFreq(freq);
    map<char, string> huffmanCode;
    generateCodes(root, "", huffmanCode);

    // Open output in BINARY mode
    ofstream outFile("out.bin", ios::binary);

    // 1. Write header: unique char count + total original characters
    outFile << freq.size() << " " << totalChars << "\n";
    for (auto pair : freq) {
        outFile << static_cast<int>(static_cast<unsigned char>(pair.first)) 
                << " " << pair.second << "\n";
    }

    // 2. Pack 8 bits into 1 byte and write raw bytes
    inFile.clear();
    inFile.seekg(0, ios::beg);

    uint8_t byteBuffer = 0;
    int bitCount = 0;

    while (inFile.get(ch)) {
        const string& code = huffmanCode[ch];
        for (char bit : code) {
            byteBuffer = (byteBuffer << 1) | (bit - '0'); // Shift bit into buffer
            bitCount++;

            if (bitCount == 8) {
                outFile.put(static_cast<char>(byteBuffer)); // Write 1 full byte
                byteBuffer = 0;
                bitCount = 0;
            }
        }
    }

    // Flush remaining bits in the final partial byte (padded with trailing 0s)
    if (bitCount > 0) {
        byteBuffer <<= (8 - bitCount);
        outFile.put(static_cast<char>(byteBuffer));
    }

    cout << "Compressed successfully to out.bin\n";
}

void decodeFile(ifstream& inFile) {
    size_t uniqueCount;
    uint64_t totalChars;
    if (!(inFile >> uniqueCount >> totalChars)) return;

    // 1. Read frequency table
    map<char, int> freq;
    for (size_t i = 0; i < uniqueCount; ++i) {
        int asciiVal, count;
        inFile >> asciiVal >> count;
        freq[static_cast<char>(asciiVal)] = count;
    }

    // Consume the single '\n' immediately after the header
    inFile.get();

    Node* root = buildTreeFromFreq(freq);
    if (!root) return;

    // 2. Read raw bytes and unpack bits one by one
    ofstream outFile("decoded.txt", ios::binary);
    Node* curr = root;
    uint64_t decodedCount = 0;
    char rawByte;

    while (inFile.get(rawByte) && decodedCount < totalChars) {
        uint8_t byte = static_cast<uint8_t>(rawByte);

        // Extract bits from Most Significant Bit (7) down to Least Significant Bit (0)
        for (int i = 7; i >= 0 && decodedCount < totalChars; --i) {
            int bit = (byte >> i) & 1;
            curr = (bit == 0) ? curr->left : curr->right;

            if (!curr->left && !curr->right) {
                outFile.put(curr->ch);
                curr = root;
                decodedCount++;
            }
        }
    }

    cout << "Decoded successfully to decoded.txt\n";
}

void usage() {
    cout << "Usage: ./huffman {-e | -d} {file}\n";
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc != 3) usage();

    string mode = argv[1];
    if (mode != "-e" && mode != "-d") usage();

    ifstream inFile(argv[2], ios::binary);
    if (!inFile) {
        cerr << "Error: Could not open file " << argv[2] << "\n";
        return 1;
    }

    if (mode == "-e") encodeFile(inFile);
    else              decodeFile(inFile);

    return 0;
}