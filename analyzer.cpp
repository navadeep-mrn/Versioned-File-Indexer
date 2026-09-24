#include <bits/stdc++.h>
using namespace std;
using namespace std::chrono;

template<typename T>
void printTopK(vector<pair<string,T>> &v,int k){
    for(int i=0;i<min(k,(int)v.size());i++){
        cout<<v[i].first<<" -> "<<v[i].second<<endl;
    }
}

class BufferedReader{
    ifstream file;
    size_t bufferSize;
    vector<char> buffer;

public:
    BufferedReader(string path,size_t kb){
        bufferSize = kb*1024;

        if(bufferSize < 256*1024 || bufferSize > 1024*1024)
            throw runtime_error("Buffer size must be between 256KB and 1024KB");

        file.open(path);
        if(!file)
            throw runtime_error("Cannot open file");

        buffer.resize(bufferSize);
    }

    bool readChunk(string &out){
        if(!file.good()) return false;

        file.read(buffer.data(),bufferSize);
        size_t readBytes=file.gcount();

        if(readBytes==0) return false;

        out.assign(buffer.data(),readBytes);
        return true;
    }
};

class Tokenizer{
    string leftover;

public:
    vector<string> tokenize(string chunk){
        vector<string> tokens;
        string current = leftover;

        for(char c:chunk){

            if(isalnum(c))
                current+=tolower(c);

            else{
                if(!current.empty()){
                    tokens.push_back(current);
                    current.clear();
                }
            }
        }

        leftover=current;
        return tokens;
    }

    string flush(){
        string t=leftover;
        leftover.clear();
        return t;
    }
};

class VersionedIndex{
    unordered_map<string, unordered_map<string,int>> versions;

public:

    void buildIndex(string version,string path,int bufferKB){

        BufferedReader reader(path,bufferKB);
        Tokenizer tokenizer;

        unordered_map<string,int> freq;
        string chunk;

        while(reader.readChunk(chunk)){
            auto tokens=tokenizer.tokenize(chunk);
            for(auto &w:tokens)
                freq[w]++;
        }

        string last=tokenizer.flush();
        if(!last.empty()) freq[last]++;

        versions[version]=freq;
    }

    int wordCount(string version,string word){

        transform(word.begin(),word.end(),word.begin(),::tolower);

        if(!versions.count(version))
            throw runtime_error("Version not found");

        return versions[version][word];
    }

    int diff(string v1,string v2,string word){

        transform(word.begin(),word.end(),word.begin(),::tolower);

        int a=0,b=0;

        if(versions.count(v1))
            a=versions[v1][word];

        if(versions.count(v2))
            b=versions[v2][word];

        return a-b;
    }

    vector<pair<string,int>> topK(string version){

        if(!versions.count(version))
            throw runtime_error("Version not found");

        vector<pair<string,int>> v;

        for(auto &p:versions[version])
            v.push_back(p);

        sort(v.begin(),v.end(),
             [](auto &a,auto &b){
                return a.second>b.second;
             });

        return v;
    }
};

class Query{
public:
    virtual void execute(VersionedIndex &idx)=0;
    virtual ~Query(){}
};

class WordQuery: public Query{

    string version;
    string word;

public:

    WordQuery(string v,string w):version(v),word(w){}

    void execute(VersionedIndex &idx){
        cout<<"Word count = "
            <<idx.wordCount(version,word)<<endl;
    }
};

class DiffQuery: public Query{

    string v1,v2,word;

public:

    DiffQuery(string a,string b,string w):v1(a),v2(b),word(w){}

    void execute(VersionedIndex &idx){
        cout<<"Difference = "
            <<idx.diff(v1,v2,word)<<endl;
    }
};

class TopQuery: public Query{

    string version;
    int k;

public:

    TopQuery(string v,int k):version(v),k(k){}

    void execute(VersionedIndex &idx){

        auto v=idx.topK(version);
        printTopK(v,k);
    }
};

string getArg(vector<string> &args,string key){

    for(int i=0;i<args.size();i++)
        if(args[i]==key && i+1<args.size())
            return args[i+1];

    return "";
}

int main(int argc,char* argv[]){

    try{

        auto start=high_resolution_clock::now();

        vector<string> args;
        for(int i=1;i<argc;i++)
            args.push_back(argv[i]);

        string query=getArg(args,"--query");
        int buffer=stoi(getArg(args,"--buffer"));

        VersionedIndex index;

        Query* q=nullptr;

        if(query=="word"){

            string file=getArg(args,"--file");
            string version=getArg(args,"--version");
            string word=getArg(args,"--word");

            index.buildIndex(version,file,buffer);

            q=new WordQuery(version,word);
        }

        else if(query=="top"){

            string file=getArg(args,"--file");
            string version=getArg(args,"--version");
            int k=stoi(getArg(args,"--top"));

            index.buildIndex(version,file,buffer);

            q=new TopQuery(version,k);
        }

        else if(query=="diff"){

            string file1=getArg(args,"--file1");
            string file2=getArg(args,"--file2");

            string v1=getArg(args,"--version1");
            string v2=getArg(args,"--version2");

            string word=getArg(args,"--word");

            index.buildIndex(v1,file1,buffer);
            index.buildIndex(v2,file2,buffer);

            q=new DiffQuery(v1,v2,word);
        }

        else
            throw runtime_error("Unknown query type");

        q->execute(index);

        auto end=high_resolution_clock::now();
        double time = duration<double>(end-start).count();

        cout<<"Buffer Size: "<<buffer<<" KB"<<endl;
        cout<<"Execution Time: "<<time<<" seconds"<<endl;

        delete q;

    }catch(exception &e){
        cout<<"Error: "<<e.what()<<endl;
    }
}