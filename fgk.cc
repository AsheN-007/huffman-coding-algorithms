#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <cstdint>

using namespace std;

constexpr int POSSIBLE_SYMBOLS = 256;

struct Node {
    bool isZero;
    bool isRoot;
    bool isLeaf;

    Node* parent;
    Node* left;
    Node* right;

    uint8_t symbol;
    int value;
    int order;

    Node(bool isZero, bool isRoot, bool isLeaf, Node* parent, uint8_t symbol, int value, int order)
        : isZero(isZero), isRoot(isRoot), isLeaf(isLeaf),
          parent(parent), left(nullptr), right(nullptr),
          symbol(symbol), value(value), order(order) {}
};

class FGKTree {
private:
    Node* root;
    Node* zeroNode;
    array<Node*, POSSIBLE_SYMBOLS> symbolNodes;

    // Recursively find the highest-order node with the exact same frequency/value
    Node* findReplaceNode(Node* currMax, Node* current) {
        Node* result = currMax;

        if (current->value > result->value && !current->isLeaf) {
            if (Node* leftRes = findReplaceNode(result, current->left)) {
                result = leftRes;
            }
            if (Node* rightRes = findReplaceNode(result, current->right)) {
                result = rightRes;
            }
        } else if (current->value == result->value && current->order > result->order) {
            result = current;
        }

        return (result != currMax) ? result : nullptr;
    }

    // Swap two nodes in the tree while keeping their fixed order numbers in place
    void swapNodes(Node* n1, Node* n2) {
        swap(n1->order, n2->order);

        if (n1->parent->left == n1) n1->parent->left = n2;
        else                        n1->parent->right = n2;

        if (n2->parent->left == n2) n2->parent->left = n1;
        else                        n2->parent->right = n1;

        swap(n1->parent, n2->parent);
    }

    void destroy(Node* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

public:
    FGKTree() {
        root = new Node(true, true, true, nullptr, 0, 0, POSSIBLE_SYMBOLS * 2);
        zeroNode = root;
        symbolNodes.fill(nullptr);
    }

    ~FGKTree() {
        destroy(root);
    }

    Node* getRoot() const { return root; }
    Node* getZeroNode() const { return zeroNode; }
    Node* getSymbolNode(uint8_t sym) const { return symbolNodes[sym]; }

    // Walk up from node to root to build its current bit path
    vector<int> getCode(Node* node) const {
        vector<int> code;
        Node* current = node;
        while (!current->isRoot) {
            Node* parent = current->parent;
            code.push_back((parent->left == current) ? 0 : 1);
            current = parent;
        }
        reverse(code.begin(), code.end());
        return code;
    }

    // Split the current NYT (zero) node into a new NYT left child and symbol right child
    Node* addSymbol(uint8_t sym) {
        Node* leftChild  = new Node(true, false, true, zeroNode, 0, 0, zeroNode->order - 2);
        Node* rightChild = new Node(false, false, true, zeroNode, sym, 1, zeroNode->order - 1);

        Node* previousZero = zeroNode;
        zeroNode->isZero = false;
        zeroNode->isLeaf = false;
        zeroNode->left   = leftChild;
        zeroNode->right  = rightChild;

        symbolNodes[sym] = rightChild;
        zeroNode = leftChild;

        return previousZero;
    }

    // Rebalance and increment weights up to the root
    void updateTree(Node* currNode) {
        while (!currNode->isRoot) {
            Node* replaceNode = findReplaceNode(currNode, root);
            if (replaceNode && currNode->parent != replaceNode) {
                swapNodes(currNode, replaceNode);
            }
            currNode->value++;
            currNode = currNode->parent;
        }
        currNode->value++;
    }
};

// Bit-level writer 
class BitWriter {
private:
    ofstream& out;
    uint8_t buffer = 0;
    int bitsRemaining = 8;

public:
    explicit BitWriter(ofstream& outFile) : out(outFile) {}

    void writeBits(const vector<int>& bits) {
        for (int b : bits) {
            uint8_t bit = static_cast<uint8_t>(b) << (bitsRemaining - 1);
            buffer |= bit;
            bitsRemaining--;

            if (bitsRemaining == 0) {
                out.put(static_cast<char>(buffer));
                buffer = 0;
                bitsRemaining = 8;
            }
        }
    }

    void writeByte(uint8_t byte) {
        int written = bitsRemaining;
        int shift = 8 - written;
        buffer |= (byte >> shift);
        out.put(static_cast<char>(buffer));

        buffer = (bitsRemaining == 8) ? 0 : static_cast<uint8_t>(byte << written);
    }

    void flush() {
        if (bitsRemaining < 8) {
            out.put(static_cast<char>(buffer));
        }
        // Final byte records how many bits are valid in the preceding byte
        uint8_t validBitsAtEnd = (bitsRemaining == 8) ? 8 : static_cast<uint8_t>(8 - bitsRemaining);
        out.put(static_cast<char>(validBitsAtEnd));
    }
};

// Bit-level reader matching BitWriter's format
class BitReader {
private:
    ifstream& in;
    uint8_t buffer = 0;
    int bitsLeft = 0;
    streamoff payloadEnd;
    int validBitsAtEnd;

public:
    explicit BitReader(ifstream& inFile) : in(inFile) {
        in.seekg(-1, ios::end);
        payloadEnd = in.tellg();

        char lastByte = 0;
        in.get(lastByte);
        validBitsAtEnd = static_cast<uint8_t>(lastByte);

        in.clear();
        in.seekg(0, ios::beg);
    }

    int readBit() {
        if (bitsLeft == 0) {
            char raw = 0;
            if (!in.get(raw)) return -1;
            buffer = static_cast<uint8_t>(raw);
            bitsLeft = 8;
        }

        streamoff pos = in.tellg();
        if (validBitsAtEnd != 8) {
            if (pos == payloadEnd && validBitsAtEnd <= (8 - bitsLeft)) {
                return -1;
            }
        }
        if (pos > payloadEnd || pos == -1) {
            return -1;
        }

        bitsLeft--;
        return (buffer >> bitsLeft) & 1;
    }

    int readByte() {
        uint8_t result = 0;
        for (int i = 0; i < 8; ++i) {
            int bit = readBit();
            if (bit == -1) return -1;
            result |= static_cast<uint8_t>(bit << (7 - i));
        }
        return result;
    }
};

void encodeFile(ifstream& inFile) {
    ofstream outFile("out.bin", ios::binary);
    if (!outFile) {
        cerr << "Error: Could not create out.bin\n";
        return;
    }

    FGKTree tree;
    BitWriter writer(outFile);

    char rawByte;
    while (inFile.get(rawByte)) {
        uint8_t sym = static_cast<uint8_t>(rawByte);
        Node* symNode = tree.getSymbolNode(sym);

        if (symNode) {
            writer.writeBits(tree.getCode(symNode));
            tree.updateTree(symNode);
        } else {
            writer.writeBits(tree.getCode(tree.getZeroNode()));
            writer.writeByte(sym);

            Node* newInternal = tree.addSymbol(sym);
            tree.updateTree(newInternal);
        }
    }

    writer.flush();
    cout << "Encoded successfully to out.bin\n";
}

void decodeFile(ifstream& inFile) {
    // Ensure file has at least the footer byte
    inFile.seekg(0, ios::end);
    if (inFile.tellg() < 1) {
        cerr << "Error: Input file is empty or corrupted.\n";
        return;
    }

    ofstream outFile("decoded.txt", ios::binary);
    if (!outFile) {
        cerr << "Error: Could not create decoded.txt\n";
        return;
    }

    FGKTree tree;
    BitReader reader(inFile);

    while (true) {
        Node* currNode = tree.getRoot();
        bool eofReached = false;

        while (!currNode->isLeaf) {
            int bit = reader.readBit();
            if (bit == 0) {
                currNode = currNode->left;
            } else if (bit == 1) {
                currNode = currNode->right;
            } else {
                eofReached = true;
                break;
            }
        }

        if (eofReached) break;

        uint8_t sym = 0;
        if (currNode->isZero) {
            int rawSym = reader.readByte();
            if (rawSym == -1) break;
            sym = static_cast<uint8_t>(rawSym);
            currNode = tree.addSymbol(sym);
        } else {
            sym = currNode->symbol;
        }

        outFile.put(static_cast<char>(sym));
        tree.updateTree(currNode);
    }

    cout << "Decoded successfully to decoded.txt\n";
}

void usage() {
    cout << "Usage: ./fgk {-e | -d} {file}\n";
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