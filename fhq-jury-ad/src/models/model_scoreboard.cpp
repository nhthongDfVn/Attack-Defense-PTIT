#include "model_scoreboard.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <utils_logger.h>

// ---------------------------------------------------------------------

ModelScoreboard::ModelScoreboard(bool bRandom, int nGameStart, int nGameEnd, const std::vector<ModelTeamConf> &vTeamsConf, const std::vector<ModelServiceConf> &vServicesConf) {
    TAG = "ModelScoreboard";
    m_bRandom = bRandom;
    std::srand(unsigned(std::time(0)));
    m_nGameStart = nGameStart;
    m_nGameEnd = nGameEnd;

    m_mapTeamsStatuses.clear(); // possible memory leak
    for (unsigned int iteam = 0; iteam < vTeamsConf.size(); iteam++) {
        ModelTeamConf teamConf = vTeamsConf[iteam];
        int nTeamNum = teamConf.num();
        m_mapTeamsStatuses[nTeamNum] = new ModelTeamStatus(nTeamNum, vServicesConf);
        m_mapTeamsStatuses[nTeamNum]->setPlace(iteam+1);

        // random values of service for testing
        if (m_bRandom) {
            double nScore = (std::rand() % 10000)/10;
            m_mapTeamsStatuses[nTeamNum]->setScore(nScore);
        }

        for (unsigned int iservice = 0; iservice < vServicesConf.size(); iservice++) {
            ModelServiceConf service = vServicesConf[iservice];
            m_mapTeamsStatuses[nTeamNum]->setServiceStatus(service.num(), ModelServiceStatus::SERVICE_DOWN);

            // random states of service for testing 
            if (m_bRandom) {
                m_mapTeamsStatuses[nTeamNum]->setServiceStatus(service.num(), randomServiceStatus()); 
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(m_mutexJson);
        m_jsonScoreboard.clear();
        for (unsigned int iteam = 0; iteam < vTeamsConf.size(); iteam++) {
            ModelTeamConf teamConf = vTeamsConf[iteam];
            int nTeamNum = teamConf.num();
            // team.num();
            nlohmann::json teamData;
            teamData["place"] = m_mapTeamsStatuses[nTeamNum]->place();
            teamData["score"] = m_mapTeamsStatuses[nTeamNum]->score();

            for (unsigned int iservice = 0; iservice < vServicesConf.size(); iservice++) {
                ModelServiceConf serviceConf = vServicesConf[iservice];
                nlohmann::json serviceData;
                serviceData["defence"] = 0;
                serviceData["attack"] = 0;
                serviceData["sla"] = 100.0;
                serviceData["status"] = m_mapTeamsStatuses[nTeamNum]->serviceStatus(serviceConf.num());
                teamData[serviceConf.id()] = serviceData;
            }
            m_jsonScoreboard[teamConf.id()] = teamData;
        }
    }

}

// ----------------------------------------------------------------------

std::string ModelScoreboard::randomServiceStatus() {
    std::string sResult = ModelServiceStatus::SERVICE_DOWN;
    int nState = std::rand() % 5;
    switch (nState) {
        case 0: sResult = ModelServiceStatus::SERVICE_UP; break;
        case 1: sResult = ModelServiceStatus::SERVICE_DOWN; break;
        case 2: sResult = ModelServiceStatus::SERVICE_MUMBLE; break;
        case 3: sResult = ModelServiceStatus::SERVICE_CORRUPT; break;
        case 4: sResult = ModelServiceStatus::SERVICE_SHIT; break;
    }
    return sResult;
}

// ----------------------------------------------------------------------

void ModelScoreboard::setServiceStatus(int nTeamNum, int nServiceNum, const std::string &sStatus) {
    std::lock_guard<std::mutex> lock(m_mutexJson);
    std::string sNewStatus = m_bRandom ? randomServiceStatus() : sStatus;

    std::map<int,ModelTeamStatus *>::iterator it;
    it = m_mapTeamsStatuses.find(nTeamNum);
    if (it != m_mapTeamsStatuses.end()) {
        if (it->second->serviceStatus(nServiceNum) != sNewStatus) {
            it->second->setServiceStatus(nServiceNum, sNewStatus);
            std::string sTeamNum = "team" + std::to_string(nTeamNum);
            std::string sServiceNum = "service" + std::to_string(nServiceNum);
            m_jsonScoreboard[sTeamNum][sServiceNum]["status"] = sNewStatus;
        }
    }
}

// ----------------------------------------------------------------------

void ModelScoreboard::incrementAttackScore(int nTeamNum, int nServiceNum) {
    // TODO
    Log::warn(TAG, "TODO: incrementAttackScore");
}

// ----------------------------------------------------------------------

void ModelScoreboard::incrementDefenceScore(int nTeamNum, int nServiceNum) {
    // TODO
    Log::warn(TAG, "TODO: incrementDefenceScore");
}

// ----------------------------------------------------------------------

std::string ModelScoreboard::serviceStatus(int nTeamNum, int nServiceNum) {
    std::map<int,ModelTeamStatus *>::iterator it;
    it = m_mapTeamsStatuses.find(nTeamNum);
    if (it != m_mapTeamsStatuses.end()) {
        return it->second->serviceStatus(nServiceNum);
    }
    return "";
}

// ----------------------------------------------------------------------

static bool sort_using_greater_than(double u, double v) {
   return u > v;
}

// ----------------------------------------------------------------------

void ModelScoreboard::setServiceScore(int nTeamNum, int nServiceNum, int nDefence, int nAttack, double nSLA) {
    std::lock_guard<std::mutex> lock(m_mutexJson);
    int nNewDefence = nDefence;
    int nNewAttack = nAttack;
    int nNewSLA = nSLA;

    if (m_bRandom) {
        nNewDefence = std::rand() % 10000;
        nNewAttack = std::rand() % 1000;
        nNewSLA = double(std::rand() % 1000) / 100;
    }

    {
        std::map<int,ModelTeamStatus *>::iterator it;
        it = m_mapTeamsStatuses.find(nTeamNum);
        if (it != m_mapTeamsStatuses.end()) {
            it->second->setServiceScore(nServiceNum, nNewDefence, nNewAttack, nNewSLA);
        }
    }

    // sort places
    {
        std::vector<double> vScores;
        std::map<int,ModelTeamStatus *>::iterator it1;
        for (it1 = m_mapTeamsStatuses.begin(); it1 != m_mapTeamsStatuses.end(); it1++) {
            if(std::find(vScores.begin(), vScores.end(), it1->second->score()) == vScores.end()) {
                vScores.push_back(it1->second->score());
            }
        }
        std::sort(vScores.begin(), vScores.end(), sort_using_greater_than);
        for (it1 = m_mapTeamsStatuses.begin(); it1 != m_mapTeamsStatuses.end(); it1++) {
            double nScore = it1->second->score();
            ptrdiff_t pos = std::find(vScores.begin(), vScores.end(), nScore) - vScores.begin();
            it1->second->setPlace(pos + 1); // TODO fix: same scores will be same place
        }
        
    }

    // update json
    {
        std::string sTeamNum = "team" + std::to_string(nTeamNum);
        std::string sServiceNum = "service" + std::to_string(nServiceNum);
        m_jsonScoreboard[sTeamNum][sServiceNum]["attack"] = nNewAttack;
        m_jsonScoreboard[sTeamNum][sServiceNum]["defence"] = nNewDefence;
        m_jsonScoreboard[sTeamNum][sServiceNum]["sla"] = nNewSLA;

        std::map<int,ModelTeamStatus *>::iterator it1;
        for (it1 = m_mapTeamsStatuses.begin(); it1 != m_mapTeamsStatuses.end(); it1++) {
            ModelTeamStatus *pTeamStatus = it1->second;
            std::string sTeamNum = "team" + std::to_string(pTeamStatus->teamNum());

            // std::cout << sTeamNum << ": result: score: " << pTeamStatus->score() << ", place: " << pTeamStatus->place() << "\n";
            m_jsonScoreboard[sTeamNum]["score"] = pTeamStatus->score();
            m_jsonScoreboard[sTeamNum]["place"] = pTeamStatus->place();
        }
    }
}

// ----------------------------------------------------------------------

double ModelScoreboard::calculateSLA(int nFlagsSuccess, const ModelServiceConf &serviceConf) {
    // TODO start game time
    double nTimeSuccess = nFlagsSuccess * serviceConf.timeSleepBetweenRunScriptsInSec();

    auto now = std::chrono::system_clock::now().time_since_epoch();
    int nLastTime = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    nLastTime = std::min(nLastTime, m_nGameEnd);
    
    double nTimeAll = (double)nLastTime - (double)m_nGameStart;
    double nTimeAll_ = nTimeAll;

    // correction
    nTimeAll = nTimeAll - std::fmod(nTimeAll, serviceConf.timeSleepBetweenRunScriptsInSec());
    nTimeAll += serviceConf.timeSleepBetweenRunScriptsInSec();

    if (nTimeAll == 0) {
        return 100.0; // Normal
    }
    
    // nTimeAll
    double nResult = (nTimeSuccess*100) / nTimeAll;
    if (nResult > 100.0) {
        // Log::err(TAG, "calculateSLA nFlagsSuccess = " + std::to_string(nFlagsSuccess) + "");
        // Log::err(TAG, "calculateSLA nTimeAll_ = " + std::to_string(nTimeAll_) + "");
        // Log::err(TAG, "calculateSLA serviceConf.timeSleepBetweenRunScriptsInSec() = " + std::to_string(serviceConf.timeSleepBetweenRunScriptsInSec()) + "");
        Log::err(TAG, "calculateSLA nResult = " + std::to_string(nResult) + "% - wrong");
        nResult = 100.0;
    }
    return nResult;
}

// ----------------------------------------------------------------------

std::string ModelScoreboard::toString(){
    // TODO mutex
    std::string sResult = ""; 
    std::map<int,ModelTeamStatus *>::iterator it;
    for (it = m_mapTeamsStatuses.begin(); it != m_mapTeamsStatuses.end(); ++it){
        sResult += "team" + std::to_string(it->first) + ": \n"
            "\tscore: " + std::to_string(it->second->score()) + "\n"
            + it->second->servicesToString() + "\n";
    }

    return sResult;
}

// ----------------------------------------------------------------------

const nlohmann::json &ModelScoreboard::toJson(){
    std::lock_guard<std::mutex> lock(m_mutexJson);
    return m_jsonScoreboard;
}

// ----------------------------------------------------------------------