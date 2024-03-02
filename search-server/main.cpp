#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    
    string line;
    getline(cin, line);
    
    return line;
}

int ReadLineWithNumber() {
    
    int result = 0;
    cin >> result;
    ReadLine();
    
    return result;
}

vector<string> SplitIntoWords(const string& TEXT) {
    
    vector<string> words;
    string word;
    
    for (const char CH : TEXT) {
        if (CH == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += CH;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
    
public:
    
    void SetStopWords(const string& TEXT) {
        
        for (const string& WORD : SplitIntoWords(TEXT)) {
            stop_words_.insert(WORD);
        }
    }
    
    void AddDocument(int document_id, const string& DOCUMENT) {
        
        const vector<string> WORDS = SplitIntoWordsNoStop(DOCUMENT);
        int N = WORDS.size();
        
        for (const string& WORD : WORDS) {
            word_to_document_freqs_[WORD][document_id]+= 1.0 / N;
        }
        ++document_count_;
    }
    
    vector<Document> FindTopDocuments(const string& RAW_QUERY) const {
        
        const Query QUERY_WORDS = ParseQuery(RAW_QUERY);
        auto matched_documents = FindAllDocuments(QUERY_WORDS);

        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 return lhs.relevance > rhs.relevance;
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:
    
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    int document_count_ = 0;
    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;
    
    bool IsStopWord(const string& WORD) const {
        return stop_words_.count(WORD) > 0;
    }
    
    vector<string> SplitIntoWordsNoStop(const string& TEXT) const {
        
        vector<string> words;
        
        for (const string& WORD : SplitIntoWords(TEXT)) {
            if (!IsStopWord(WORD)) {
                words.push_back(WORD);
            }
        }
        return words;
    }
    
    Query ParseQuery(const string& TEXT) const {
        
        Query query;
        
        for (string& WORD : SplitIntoWordsNoStop(TEXT)) {
            
            if (WORD[0] == '-') {
                WORD = WORD.substr(1);
                
                if (!IsStopWord(WORD)) {
                query.minus_words.insert(WORD);
                }
            }
            
                query.plus_words.insert(WORD);
        }
    
        return query;
    }

    vector<Document> FindAllDocuments(const Query& QUERY_WORDS) const {
        
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;
        
        for (const string& WORD : QUERY_WORDS.plus_words) {
            
            if (word_to_document_freqs_.count(WORD) == 0) {
                continue;
            }
            
            const double IDF = log(static_cast <double> (document_count_) /         word_to_document_freqs_.at(WORD).size());
            
            for (const auto& [DOCUMENT_ID, TF] : word_to_document_freqs_.at(WORD)) { 
                
                document_to_relevance[DOCUMENT_ID] += IDF * TF;
            }
        }
        
        for (const string& WORD : QUERY_WORDS.minus_words) {
            
            if (word_to_document_freqs_.count(WORD) == 0) {
                continue;
            }
            
            for (const auto& [DOCUMENT_ID, TF] : word_to_document_freqs_.at(WORD)) { 
                
                document_to_relevance.erase(DOCUMENT_ID);
            }
        }

        for (auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance });
        }
        
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int DOCUMENT_COUNT = ReadLineWithNumber();
    
    for (int document_id = 0; document_id < DOCUMENT_COUNT; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    
    const SearchServer search_server = CreateSearchServer();
    const string QUERY = ReadLine();
    
for (const auto& [DOCUMENT_ID, RELEVANCE] : search_server.FindTopDocuments(QUERY)) {
        
      cout << "{ document_id = "s << DOCUMENT_ID << ", "
           << "relevance = "s << RELEVANCE << " }"s << endl;
  }
    
    return 0;
}