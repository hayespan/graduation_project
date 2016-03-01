#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

namespace HayComm {

    Config::Config() :m_pFilePtr(NULL){
    };

    int Config::InitConfigFile(const string & sFileName) {
        m_sFileName = sFileName;
        if (m_pFilePtr) fclose(m_pFilePtr), m_pFilePtr = NULL;
        m_oBody.clear();
        
        FILE * pTmp = fopen(sFileName.c_str(), "r");
        if (!pTmp) {
            return -1;
        }
        m_pFilePtr = pTmp, pTmp = NULL;
        int iRet = ParseConfigFile(m_pFilePtr, m_oBody);
        if (iRet < 0) {
            return -2;
        } 
        return 0;
    }

    Body & Config::GetConfigBody() {
        return m_oBody;
    }

    int Config::GetValue(const string & sSectionName, const string & sKeyName, string & sValue) {
        sValue = "";
        for (size_t i=0; i<m_oBody.size(); ++i) {
            if (m_oBody[i].first == sSectionName) {
                Section & oSection = m_oBody[i];
                for (size_t j=0; j<oSection.second.size(); ++j) {
                    if (oSection.second[j].first == sKeyName) {
                        sValue = oSection.second[j].second;
                        return 0;
                    }
                }
            }
        }
        return -1;
    }

    int Config::UpdateConfigFile(const string & sOthFileName) {
        string sDestFileName;
        if (sOthFileName.size() > 0) {
            sDestFileName = sOthFileName;
        } else {
            sDestFileName = m_sFileName;
        }
        FILE * pForWrite = fopen(sDestFileName.c_str(), "w+");
        if (!pForWrite) {
            return -1;
        }
        int iErrLineNum = -1;
        for (size_t i=0; i<m_oBody.size(); ++i) {
            Section & oSection = m_oBody[i];
            string & sSectionName = oSection.first;
            vector<KvPair> & oPairs = oSection.second;
            string sFormatSectionName = "[" + sSectionName + "]\n"; 
            if (1 != fwrite(sFormatSectionName.c_str(), sFormatSectionName.size(), 1, pForWrite)) {
                iErrLineNum = __LINE__;
                goto WRITE_ERR;
            }
            for (size_t j=0; j<oPairs.size(); ++j) {
                KvPair & oKvPair = oPairs[j];
                string & sKeyName = oKvPair.first;
                string & sValue = oKvPair.second;
                string sFormatKvPair = sKeyName + "=" + sValue + "\n";
                if (1 != fwrite(sFormatKvPair.c_str(), sFormatKvPair.size(), 1, pForWrite)) {
                    iErrLineNum = __LINE__;
                    goto WRITE_ERR;
                }
            }
            if (1 != fwrite("\n", 1, 1, pForWrite)) {
                iErrLineNum = __LINE__;
                goto WRITE_ERR;
            }
        }
        fclose(pForWrite);
        return 0;

WRITE_ERR:
        printf("line[%d]\n", iErrLineNum);
        fclose(pForWrite);
        return -2;
    }

    int Config::ParseConfigFile(FILE * pFilePtr, Body & oBody) {
        if (!pFilePtr) return -1;
        oBody.clear();
        struct stat st;
        int iFd = fileno(pFilePtr);
        if (-1 == fstat(iFd, &st)) {
            return -2;
        }
        long iSize = st.st_size;
        if (iSize == 0) {
            return 0;
        }
        char * pBuf = (char *)malloc(iSize);
        int iCur = 0;

        int iSecTkL = -1;
        int iKeyTkL = -1;
        int iValTkL = -1;
        int iComTkL = -1;

        int iErrLineNum = -1;

        string sSectionName, sKeyName, sValue;

        if (1 != fread(pBuf, iSize, 1, pFilePtr)) {
            iErrLineNum = __LINE__;
            goto PARSER_ERR;
        }
        while (iCur < iSize) {
            char & c = pBuf[iCur];
            if (c == '\n') {
                if (iValTkL > -1) {
                    sValue = string(pBuf+iValTkL, pBuf+iCur); 
                    iValTkL = -1;
                    KvPair * pTmpKvPair = GetLastKvPair(GetLastSection(&oBody));
                    if (!pTmpKvPair) {
                        iErrLineNum = __LINE__;
                        goto PARSER_ERR;
                    }
                    (*pTmpKvPair).second = sValue;
                } else if (iSecTkL > -1 ||
                        iKeyTkL > -1) {
                    iErrLineNum = __LINE__;
                    goto PARSER_ERR;
                } else if (iComTkL > -1) {
                    iComTkL = -1;
                }
                
            } else if (c == ' ') {
            } else if (c == '[') {
                if (iSecTkL > -1) {
                    //
                } else if (iKeyTkL > -1) {
                    //
                } else if (iValTkL > -1) {
                    //
                } else {
                    iSecTkL = iCur+1;
                }

            } else if (c == ']') {
                if (iSecTkL > -1) {
                    string sTmpSectionName = string(pBuf+iSecTkL, pBuf+iCur);
                    Section oTmpSection;
                    oTmpSection.first = sTmpSectionName;
                    oBody.push_back(oTmpSection);
                    iSecTkL = -1;
                } 
            } else if (c == '#') {
                if (iSecTkL > -1 || 
                    iKeyTkL > -1 ||
                    iValTkL > -1 ||
                    iComTkL > -1
                    ) {
                    //
                } else {
                    iComTkL = iCur;
                }
            } else if (c == '=') {
                if (iSecTkL > -1)  {
                } else if (iKeyTkL > -1) {
                    string sTmpKeyName = string(pBuf+iKeyTkL, pBuf+iCur);
                    Section * pSection = GetLastSection(&oBody);
                    if (!pSection) {
                        iErrLineNum = __LINE__;
                        goto PARSER_ERR;
                    }
                    (*pSection).second.push_back(make_pair(sTmpKeyName, ""));
                    iKeyTkL = -1;
                    iValTkL = iCur+1;
                } else if (iValTkL > -1 ||
                        iComTkL > -1) {
                }
            } else {
                if (iSecTkL > -1 ||
                    iKeyTkL > -1 ||
                    iValTkL > -1 ||
                    iComTkL > -1) {
                } else {
                    Section * pSection = GetLastSection(&oBody);
                    if (!pSection) {
                        iErrLineNum = __LINE__;
                        goto PARSER_ERR;
                    }
                    iKeyTkL = iCur;
                } 
            }
            iCur++;
        }

        free(pBuf), pBuf = NULL;
        return 0;
PARSER_ERR:
        // printf("#DBG: %d %d\n", iCur, iErrLineNum);
        (void)iErrLineNum;
        free(pBuf), pBuf = NULL;
        return -3;
    }

    Config::~Config() {
        if (m_pFilePtr) {
            fclose(m_pFilePtr);
            m_pFilePtr = NULL;
        }
        m_oBody.clear();
    }

    Section * Config::GetLastSection(Body * pBody) {
        if (!pBody || (*pBody).empty()) {
            return NULL;
        } 
        return &(pBody->back());
    }

    KvPair * Config::GetLastKvPair(Section * pSection) {
        if (!pSection || (*pSection).second.empty()) {
            return NULL;
        }
        return &((*pSection).second.back());
    }
}
