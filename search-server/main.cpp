#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
/*  В задачах практикума экономилась каждая строчка, не пропускалась ни одна (почти). Я бы так вообще разделял пустой 
строкой всё, что только можно. Боялся переборщить.
    Мне нравится писать return ровно под типом возвращаемого элемента функции. Но если и здесь требуется сделать 
отступ в 4 пробела, то исправлю. Но не гарантирую, что не продолжу этого делать в задачах не на ревью. =)) */
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() 
{    
    string line;
    getline(cin, line);
    
return line;
}

int ReadLineWithNumber() 
{    
    int result = 0;
    cin >> result;

    ReadLine();

return result;
}

vector<string> SplitIntoWords(const string& text) 
{    
    vector<string> words;
    string word;
    
        for (const char ch : text) 
        {
            if (ch == ' ') 
            {
                if (!word.empty()) 
                {
                    words.push_back(word);
                    word.clear();
                }
            } 
            else 
            {
                word += ch;
            }
        }

        if (!word.empty()) 
        {
            words.push_back(word);
        }

return words;
}

struct Document 
{
    int id;
    double relevance;
};

class SearchServer 
{    
public:
    
    void SetStopWords(const string& text) 
    {
        for (const string& word : SplitIntoWords(text)) 
        {
            stop_words_.insert(word);
        }
    }
    
    void AddDocument(int document_id, const string& document) 
    {    
        const vector<string> words = SplitIntoWordsNoStop(document);
        int N = words.size();
        
            for (const string& word : words) 
            {
                word_to_document_freqs_[word][document_id]+= 1.0 / N;
            }
        ++document_count_;
    }
    
    vector<Document> FindTopDocuments(const string& raw_query) const 
    {    
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(), 
        [](const Document& lhs, const Document& rhs)
        {
        return lhs.relevance > rhs.relevance;
        }   );

            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
            {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }

    return matched_documents;
    }

private:
    
    struct Query 
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    int document_count_ = 0;
    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;
    
    bool IsStopWord(const string& word) const 
    {
    return stop_words_.count(word) > 0;
    }
    
    vector<string> SplitIntoWordsNoStop(const string& text) const 
    {    
        vector<string> words;
        
            for (const string& word : SplitIntoWords(text)) 
            {
                if (!IsStopWord(word)) 
                {
                    words.push_back(word);
                }
            }

    return words;
    }
    
    Query ParseQuery(const string& text) const 
    {    
        Query query;
        
            for (string& word : SplitIntoWordsNoStop(text)) 
            {    
                if (word[0] == '-') 
                {
                    word = word.substr(1);
                
                    if (!IsStopWord(word)) 
                    {
                    query.minus_words.insert(word);
                    }
                }
                query.plus_words.insert(word);
            }

    return query;
    }

    vector<Document> FindAllDocuments(const Query& query_words) const 
    {    
        vector<Document> matched_documents;
        map<int, double> document_to_relevance;
        
            for (const string& word : query_words.plus_words) 
            {    
                if (word_to_document_freqs_.count(word) == 0) 
                {   
                    continue;
                }

                const double IDF = log(static_cast <double> (document_count_) 
                                    / word_to_document_freqs_.at(word).size());
            
                    for (const auto& [document_id, TF] : word_to_document_freqs_.at(word)) 
                    {     
                        document_to_relevance[document_id] += IDF * TF;
                    }
            }
        
            for (const string& word : query_words.minus_words) 
            {    
                if (word_to_document_freqs_.count(word) == 0) 
                {
                    continue;
                }

                    for (const auto& [document_id, TF] : word_to_document_freqs_.at(word)) 
                    {     
                        document_to_relevance.erase(document_id);
                    }
            }

            for (auto& [document_id, relevance] : document_to_relevance) 
            {
                matched_documents.push_back({ document_id, relevance });
            }

    return matched_documents;
    }
};

SearchServer CreateSearchServer() 
{    
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());
    const int document_count = ReadLineWithNumber();
    
        for (int document_id = 0; document_id < document_count; ++document_id) 
        {
            search_server.AddDocument(document_id, ReadLine());
        }

return search_server;
}

int main() 
{    
    const SearchServer search_server = CreateSearchServer();
    const string query = ReadLine();
    
        for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) 
        {        
            cout << "{ document_id = "s << document_id << ", " << "relevance = "s << relevance << " }"s << endl;
        }

return 0;
}