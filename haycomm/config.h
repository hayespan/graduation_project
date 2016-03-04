#pragma once

#include <string>
#include <vector>

using std::string;
using std::vector;
using std::pair;

namespace HayComm {

    typedef pair<string, string> KvPair;
    typedef pair<string, vector<KvPair> > Section; 
    typedef vector<Section> Body;

    class Config {
    public:
        Config();

        int InitConfigFile(const string & sFileName); 
        string GetConfigFilePath() const;
        // save config from mem to file 
        int UpdateConfigFile(const string & sOthFileName = "");

        // get value of secion->key
        int GetValue(const string & sSectionName, const string & sKeyName, string & sValue);
        // get body, can change config 
        Body & GetConfigBody();
        // parse config file
        int ParseConfigFile(FILE * pFilePtr, Body & oBody);


        virtual ~Config();

    private:
        Section * GetLastSection(Body * pBody);
        KvPair * GetLastKvPair(Section * pSection);
    
    private:
        Config & operator=(const Config &);
        Config(const Config &);

        string m_sFileName;
        FILE * m_pFilePtr;
        Body m_oBody; 
    };

};
