#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <filesystem>

// Trie node structure for efficient keyword matching
class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    bool isEndOfWord;
    std::string word;
    
    TrieNode() : isEndOfWord(false) {}
    
    ~TrieNode() {
        for (auto& pair : children) {
            delete pair.second;
        }
    }
};

// Graph structure for representing relationships between flagged terms
class Graph {
private:
    std::unordered_map<std::string, std::vector<std::string>> adjacencyList;
    
public:
    // Add a connection between two words
    void addEdge(const std::string& word1, const std::string& word2) {
        adjacencyList[word1].push_back(word2);
        adjacencyList[word2].push_back(word1); // Undirected graph
    }
    
    // Get related words using BFS
    std::vector<std::string> getRelatedWords(const std::string& startWord, int maxDepth = 2) {
        std::unordered_set<std::string> visited;
        std::queue<std::pair<std::string, int>> queue; // Word, depth
        std::vector<std::string> relatedWords;
        
        queue.push({startWord, 0});
        visited.insert(startWord);
        
        while (!queue.empty()) {
            auto [currentWord, depth] = queue.front();
            queue.pop();
            
            if (depth > 0) { // Don't include the starting word
                relatedWords.push_back(currentWord);
            }
            
            if (depth < maxDepth) {
                for (const auto& neighbor : adjacencyList[currentWord]) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        queue.push({neighbor, depth + 1});
                    }
                }
            }
        }
        
        return relatedWords;
    }
    
    // Visualize graph connections for a specific word
    void visualizeConnections(const std::string& word) {
        if (adjacencyList.find(word) == adjacencyList.end()) {
            std::cout << "No connections found for word: " << word << std::endl;
            return;
        }
        
        std::cout << "Connections for '" << word << "':" << std::endl;
        std::cout << word << " -> ";
        
        bool first = true;
        for (const auto& neighbor : adjacencyList[word]) {
            if (!first) std::cout << ", ";
            std::cout << neighbor;
            first = false;
        }
        std::cout << std::endl;
    }

    // Save graph to file for persistence
    void saveToFile(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }

        for (const auto& [word, neighbors] : adjacencyList) {
            file << word;
            for (const auto& neighbor : neighbors) {
                file << "," << neighbor;
            }
            file << std::endl;
        }
        
        std::cout << "Graph relationships saved to " << filename << std::endl;
    }

    // Load graph from file
    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Graph relationship file not found: " << filename << std::endl;
            return;
        }

        adjacencyList.clear();
        std::string line;
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string word, neighbors;
            
            if (std::getline(iss, word, ',')) {
                while (std::getline(iss, neighbors, ',')) {
                    if (!neighbors.empty()) {
                        adjacencyList[word].push_back(neighbors);
                    }
                }
            }
        }
        
        std::cout << "Graph relationships loaded from " << filename << std::endl;
    }
};

// Content Moderation System class
class ContentModerationSystem {
private:
    TrieNode* root;
    Graph termRelationships;
    std::unordered_map<std::string, int> flaggedTermsFrequency;
    std::vector<std::pair<std::string, std::vector<std::string>>> flaggedContent;
    std::string dataDirectory;
    
    // Insert a word into the Trie
    void insertWord(const std::string& word) {
        TrieNode* current = root;
        
        for (char c : word) {
            if (current->children.find(c) == current->children.end()) {
                current->children[c] = new TrieNode();
            }
            current = current->children[c];
        }
        
        current->isEndOfWord = true;
        current->word = word;
    }
    
    // Search for a word in the Trie
    bool searchWord(const std::string& word) {
        TrieNode* current = root;
        
        for (char c : word) {
            if (current->children.find(c) == current->children.end()) {
                return false;
            }
            current = current->children[c];
        }
        
        return current->isEndOfWord;
    }
    
    // Search for flagged words in a given text
    std::vector<std::string> searchFlaggedWords(const std::string& text) {
        std::vector<std::string> flaggedWords;
        std::string lowerText = text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
        
        std::istringstream iss(lowerText);
        std::string word;
        
        while (iss >> word) {
            // Remove punctuation
            word.erase(std::remove_if(word.begin(), word.end(), 
                [](char c) { return std::ispunct(static_cast<unsigned char>(c)); }), word.end());
            
            if (searchWord(word)) {
                flaggedWords.push_back(word);
                flaggedTermsFrequency[word]++;
            }
        }
        
        return flaggedWords;
    }
    
    // Save banned words to file
    void saveBannedWords(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        
        // Perform a DFS traversal of the Trie to get all words
        std::function<void(TrieNode*, std::string)> dfs = [&](TrieNode* node, std::string prefix) {
            if (node->isEndOfWord) {
                file << prefix << std::endl;
            }
            
            for (const auto& [ch, child] : node->children) {
                dfs(child, prefix + ch);
            }
        };
        
        dfs(root, "");
        std::cout << "Banned words saved to " << filename << std::endl;
    }
    
    // Save statistics to file
    void saveStatistics(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file for writing: " << filename << std::endl;
            return;
        }
        
        for (const auto& [term, freq] : flaggedTermsFrequency) {
            file << term << "," << freq << std::endl;
        }
        
        std::cout << "Statistics saved to " << filename << std::endl;
    }
    
    // Load statistics from file
    void loadStatistics(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Statistics file not found: " << filename << std::endl;
            return;
        }
        
        flaggedTermsFrequency.clear();
        std::string line;
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string term;
            int freq;
            
            if (std::getline(iss, term, ',')) {
                iss >> freq;
                flaggedTermsFrequency[term] = freq;
            }
        }
        
        std::cout << "Statistics loaded from " << filename << std::endl;
    }
    
public:
    ContentModerationSystem(const std::string& dataDir = "./data") : dataDirectory(dataDir) {
        root = new TrieNode();
        
        // Create data directory if it doesn't exist
        std::filesystem::create_directories(dataDirectory);
    }
    
    ~ContentModerationSystem() {
        delete root;
