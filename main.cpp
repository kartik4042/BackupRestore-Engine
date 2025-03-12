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
};

// Content Moderation System class
class ContentModerationSystem {
private:
    TrieNode* root;
    Graph termRelationships;
    std::unordered_map<std::string, int> flaggedTermsFrequency;
    std::vector<std::pair<std::string, std::vector<std::string>>> flaggedContent;
    
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
    
public:
    ContentModerationSystem() {
        root = new TrieNode();
    }
    
    ~ContentModerationSystem() {
        delete root;
    }
    
    // Load banned words from file
    void loadBannedWords(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filename << std::endl;
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Remove any whitespace
            line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
            
            if (!line.empty()) {
                std::transform(line.begin(), line.end(), line.begin(), ::tolower);
                insertWord(line);
            }
        }
        
        std::cout << "Banned words loaded successfully." << std::endl;
    }
    
    // Add relationships between terms
    void addTermRelationship(const std::string& term1, const std::string& term2) {
        termRelationships.addEdge(term1, term2);
    }
    
    // Flag content if it contains banned words
    bool flagContent(const std::string& content) {
        std::vector<std::string> flagged = searchFlaggedWords(content);
        
        if (!flagged.empty()) {
            flaggedContent.push_back({content, flagged});
            return true;
        }
        
        return false;
    }
    
    // Process and analyze content
    void processContent(const std::string& content) {
        std::cout << "\n====== CONTENT ANALYSIS ======" << std::endl;
        std::cout << "Content: \"" << content << "\"" << std::endl;
        
        bool isFlagged = flagContent(content);
        
        if (isFlagged) {
            std::cout << "STATUS: FLAGGED" << std::endl;
            
            const auto& lastFlagged = flaggedContent.back();
            std::cout << "Flagged terms:" << std::endl;
            
            for (const auto& term : lastFlagged.second) {
                std::cout << "- \"" << term << "\"";
                
                // Show the frequency of this term
                std::cout << " (Occurrence frequency: " << flaggedTermsFrequency[term] << ")" << std::endl;
                
                // Find related terms
                std::vector<std::string> relatedTerms = termRelationships.getRelatedWords(term);
                if (!relatedTerms.empty()) {
                    std::cout << "  Related terms: ";
                    for (size_t i = 0; i < relatedTerms.size(); ++i) {
                        if (i > 0) std::cout << ", ";
                        std::cout << relatedTerms[i];
                    }
                    std::cout << std::endl;
                }
            }
        } else {
            std::cout << "STATUS: APPROVED (No banned words detected)" << std::endl;
        }
        
        std::cout << "=============================" << std::endl;
    }
    
    // Get user feedback on flagged content
    void collectFeedback() {
        if (flaggedContent.empty()) {
            std::cout << "No flagged content to review." << std::endl;
            return;
        }
        
        const auto& lastFlagged = flaggedContent.back();
        std::cout << "\n====== FEEDBACK REQUEST ======" << std::endl;
        std::cout << "Is the flagging correct for: \"" << lastFlagged.first << "\"?" << std::endl;
        std::cout << "1. Yes, correct flagging" << std::endl;
        std::cout << "2. No, this is a false positive" << std::endl;
        
        int choice;
        std::cin >> choice;
        std::cin.ignore(); // Clear newline
        
        if (choice == 2) {
            std::cout << "Thank you for your feedback. This will help improve the system." << std::endl;
            // In a real system, this would be logged for model improvement
        } else {
            std::cout << "Thank you for confirming." << std::endl;
        }
    }
    
    // Visualize the graph of term relationships
    void visualizeTermGraph() {
        std::cout << "\n====== TERM RELATIONSHIPS ======" << std::endl;
        
        for (const auto& [term, _] : flaggedTermsFrequency) {
            termRelationships.visualizeConnections(term);
        }
        
        std::cout << "===============================" << std::endl;
    }
    
    // Add a new banned word to the system
    void addBannedWord(const std::string& word) {
        std::string lowerWord = word;
        std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
        insertWord(lowerWord);
        std::cout << "Added \"" << lowerWord << "\" to banned words list." << std::endl;
    }
    
    // Show statistics
    void showStatistics() {
        std::cout << "\n====== MODERATION STATISTICS ======" << std::endl;
        std::cout << "Total flagged content: " << flaggedContent.size() << std::endl;
        
        std::cout << "Top flagged terms:" << std::endl;
        std::vector<std::pair<std::string, int>> terms(flaggedTermsFrequency.begin(), flaggedTermsFrequency.end());
        std::sort(terms.begin(), terms.end(), 
            [](const auto& a, const auto& b) { return a.second > b.second; });
        
        int count = 0;
        for (const auto& [term, freq] : terms) {
            std::cout << "- \"" << term << "\": " << freq << " times" << std::endl;
            if (++count >= 5) break; // Show top 5
        }
        
        std::cout << "=================================" << std::endl;
    }
};

// Create a banned words file with sample content
void createSampleBannedWordsFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error creating file: " << filename << std::endl;
        return;
    }
    
    file << "hate\nscam\nfraud\nracism\nabuse\nviolence\nbullying\ndiscrimination\n";
    file.close();
    
    std::cout << "Sample banned words file created: " << filename << std::endl;
}

int main() {
    std::cout << "==== Content Moderation System ====\n" << std::endl;
    
    // Create and initialize the content moderation system
    ContentModerationSystem cms;
    
    // Create sample banned words file
    std::string bannedWordsFile = "banned_words.txt";
    createSampleBannedWordsFile(bannedWordsFile);
    
    // Load banned words
    cms.loadBannedWords(bannedWordsFile);
    
    // Add term relationships (graph edges)
    cms.addTermRelationship("hate", "racism");
    cms.addTermRelationship("hate", "discrimination");
    cms.addTermRelationship("racism", "discrimination");
    cms.addTermRelationship("scam", "fraud");
    cms.addTermRelationship("abuse", "violence");
    cms.addTermRelationship("abuse", "bullying");
    
    // Main program loop
    std::string input;
    
    while (true) {
        std::cout << "\nMenu:" << std::endl;
        std::cout << "1. Analyze content" << std::endl;
        std::cout << "2. View term relationships" << std::endl;
        std::cout << "3. Add banned word" << std::endl;
        std::cout << "4. Show statistics" << std::endl;
        std::cout << "5. Exit" << std::endl;
        std::cout << "Choose an option: ";
        
        int choice;
        std::cin >> choice;
        std::cin.ignore(); // Clear newline
        
        switch (choice) {
            case 1:
                std::cout << "Enter content to analyze: ";
                std::getline(std::cin, input);
                cms.processContent(input);
                cms.collectFeedback();
                break;
                
            case 2:
                cms.visualizeTermGraph();
                break;
                
            case 3:
                std::cout << "Enter new banned word: ";
                std::getline(std::cin, input);
                cms.addBannedWord(input);
                break;
                
            case 4:
                cms.showStatistics();
                break;
                
            case 5:
                std::cout << "Exiting program. Goodbye!" << std::endl;
                return 0;
                
            default:
                std::cout << "Invalid option. Please try again." << std::endl;
        }
    }
    
    return 0;
}
