
#include "employ_flags.h"
#include <wsjcpp_core.h>
#include <fstream>

// ---------------------------------------------------------------------
// Flag
Flag::Flag() {
    
}

// ---------------------------------------------------------------------

void Flag::generateRandomFlag(int nTimeFlagLifeInMin, const std::string &sTeamId, const std::string &sServiceId) {
    generateId();
    generateValue();

    // __int64
    long nTimeStart = WsjcppCore::currentTime_milliseconds();
    // std::cout << "nTimeStart: " << nTimeStart << "\n";
    long nTimeEnd = nTimeStart + nTimeFlagLifeInMin*60*1000;
    setTimeStart(nTimeStart);
    setTimeEnd(nTimeEnd);
    m_sTeamStole = "";
    m_sTeamId = sTeamId;
    m_sServiceId = sServiceId;
}

// ---------------------------------------------------------------------

void Flag::setId(const std::string &sId) {
    m_sId = sId;
}

// ---------------------------------------------------------------------

void Flag::generateId() {
    static const std::string sAlphabet =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

    std::string sFlagId = "";
    for (int i = 0; i < 10; ++i) {
        sFlagId += sAlphabet[rand() % sAlphabet.length()];
    }
    setId(sFlagId);
}

// ---------------------------------------------------------------------

void Flag::generateValue() {
    // TODO redesign more freeble format
    static const std::string sAlphabet = "0123456789abcdef";
    char sUuid[37];
    memset(&sUuid, '\0', 37);
    sUuid[8] = '-';
    sUuid[13] = '-';
    sUuid[18] = '-';
    sUuid[23] = '-';

    for(int i = 0; i < 36; i++){
        if (i != 8 && i != 13 && i != 18 && i != 23) {
            sUuid[i] = sAlphabet[rand() % sAlphabet.length()];
        }
    }
    setValue(std::string(sUuid));
}

// ---------------------------------------------------------------------

std::string Flag::id() const {
    return m_sId;
}

// ---------------------------------------------------------------------

void Flag::setValue(const std::string &sValue) {
    m_sValue = sValue;
}

// ---------------------------------------------------------------------

std::string Flag::value() const {
    return m_sValue;
}

// ---------------------------------------------------------------------

void Flag::setTeamId(const std::string &sTeamId) {
    m_sTeamId = sTeamId;
}

// ---------------------------------------------------------------------

const std::string &Flag::teamId() const {
    return m_sTeamId;
}

// ---------------------------------------------------------------------

void Flag::setServiceId(const std::string &sServiceId) {
    m_sServiceId = sServiceId;
}

// ---------------------------------------------------------------------

const std::string &Flag::serviceId() const {
    return m_sServiceId;
}

// ---------------------------------------------------------------------

void Flag::setTimeStart(long nTimeStart) {
    m_nTimeStart = nTimeStart;
}

// ---------------------------------------------------------------------

long Flag::timeStart() const {
    return m_nTimeStart;
}

// ---------------------------------------------------------------------

void Flag::setTimeEnd(long nTimeEnd) {
    m_nTimeEnd = nTimeEnd;
}

// ---------------------------------------------------------------------

long Flag::timeEnd() const {
    return m_nTimeEnd;
}

// ---------------------------------------------------------------------

void Flag::setTeamStole(const std::string &sTeamStole) {
    m_sTeamStole = sTeamStole;
}

// ---------------------------------------------------------------------

const std::string & Flag::teamStole() const {
    return m_sTeamStole;
}

// ---------------------------------------------------------------------

void Flag::copyFrom(const Flag &flag) {
    this->setId(flag.id());
    this->setValue(flag.value());
    this->setServiceId(flag.serviceId());
    this->setTeamId(flag.teamId());
    this->setTeamStole(flag.teamStole());
    this->setTimeStart(flag.timeStart());
    this->setTimeEnd(flag.timeEnd());
}


// ---------------------------------------------------------------------
// TableWriteOnly

TableWriteOnly::TableWriteOnly(const std::string &sDir, long nRotateAfter) {
    std::string TAG = "TableWriteOnly(" + sDir + ")";
    m_sDir = sDir;
    m_nCount = 0;
    m_nRoteteWhen = nRotateAfter;
    if (!WsjcppCore::dirExists(m_sDir)) {
        WsjcppCore::makeDir(m_sDir);
    }

    if (!WsjcppCore::dirExists(m_sDir)) {
        WsjcppLog::throw_err(TAG, "Could not found directory '" + m_sDir + "'");
    }

    // read count
    m_sFilepathCount = m_sDir + "/count_records";
    if (WsjcppCore::fileExists(m_sFilepathCount)) {
        std::string sContent;
        WsjcppCore::readTextFile(m_sFilepathCount, sContent);
        m_nCount = std::atoi(sContent.c_str());
    }
    doRotateFilename();
    m_ofsData.open(m_sFilepathData.c_str(), std::ofstream::out | std::ofstream::app);
}

// ---------------------------------------------------------------------

TableWriteOnly::~TableWriteOnly() {
    m_ofsData.close();
}

// ---------------------------------------------------------------------

void TableWriteOnly::append(const std::string &sLine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nCount++;
    if (doRotateFilename()) {
        m_ofsData.close();
        m_ofsData.open(m_sFilepathData.c_str(), std::ofstream::out | std::fstream::app);
    }
    m_ofsData << m_nCount << " " << sLine << "\n";
    m_ofsData.flush();
    WsjcppCore::writeFile(m_sFilepathCount, std::to_string(m_nCount));
}

// ---------------------------------------------------------------------

long TableWriteOnly::count() {
    return m_nCount;
}

// ---------------------------------------------------------------------

bool TableWriteOnly::doRotateFilename() {
    if (m_nCount % m_nRoteteWhen == 0) {
        long nMin = (m_nCount / m_nRoteteWhen) * m_nRoteteWhen;
        std::string sMin = std::to_string(nMin);
        while (sMin.length() < 6) {
            sMin = "0" + sMin;
        }
        long nMax = nMin + m_nRoteteWhen;
        std::string sMax = std::to_string(nMax);
        while (sMax.length() < 6) {
            sMax = "0" + sMax;
        }
        m_sFilepathData = m_sDir + "/data_" + sMin + "_" + sMax;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------
// TableFlagsAttempt

TableFlagsAttempt::TableFlagsAttempt(const std::string &sDir) 
: TableWriteOnly(sDir, 1000) {
    // nothing
}

// ---------------------------------------------------------------------

void TableFlagsAttempt::appendAttempt(const std::string &sTeamId, const std::string &sFlag) {
    this->append(
        " attempt " + sTeamId
        + " " + std::to_string(WsjcppCore::currentTime_milliseconds())
        + " " + sFlag
    );
}

// ---------------------------------------------------------------------
// TableFlagsPutFails

TableFlagsPutFails::TableFlagsPutFails(const std::string &sDir) 
: TableWriteOnly(sDir, 1000) {
    // nothing
}

// ---------------------------------------------------------------------

void TableFlagsPutFails::appendFlag(const Flag &flag, const std::string &sReason) {
    this->append(
        " check_fails " + flag.serviceId()
        + " " + flag.teamId()
        + " " + flag.id()
        + " " + flag.value()
        + std::to_string(flag.timeStart()) + " "
        + std::to_string(flag.timeEnd()) + " "
        + " " + flag.teamStole() + " "
        + " " + sReason
    );
}

// ---------------------------------------------------------------------
// TableFlagsCheckFails

TableFlagsCheckFails::TableFlagsCheckFails(const std::string &sDir) 
: TableWriteOnly(sDir, 1000) {
    // nothing
}

// ---------------------------------------------------------------------

void TableFlagsCheckFails::appendFlag(const Flag &flag, const std::string &sReason) {
    this->append(
        " check_fails " + flag.serviceId()
        + " " + flag.teamId()
        + " " + flag.id()
        + " " + flag.value()
        + std::to_string(flag.timeStart()) + " "
        + std::to_string(flag.timeEnd()) + " "
        + " " + flag.teamStole() + " "
        + " " + sReason
    );
}

// ---------------------------------------------------------------------
// EmployFlags

REGISTRY_WJSCPP_EMPLOY(EmployFlags)

EmployFlags::EmployFlags() 
: WsjcppEmployBase(EmployFlags::name(), {}) {
    TAG = EmployFlags::name();
}

// ---------------------------------------------------------------------

bool EmployFlags::init() {
    if (m_sDirectory == "") {
        WsjcppLog::info(TAG, "You must setDerectory before init");
        return false;
    }

    std::string sFlagsAttempts = WsjcppCore::doNormalizePath(m_sDirectory + "/attempts");
    if (!WsjcppCore::dirExists(sFlagsAttempts)) {
        WsjcppCore::makeDir(sFlagsAttempts);
    }

    std::string sFlagsPutFails = WsjcppCore::doNormalizePath(m_sDirectory + "/put_fails");
    m_pFlagsPutFails = new TableFlagsPutFails(sFlagsPutFails);

    std::string sFlagsCheckFails = WsjcppCore::doNormalizePath(m_sDirectory + "/check_fails");
    m_pFlagsCheckFails = new TableFlagsCheckFails(sFlagsCheckFails);

    return true;
}

// ---------------------------------------------------------------------

bool EmployFlags::deinit() {
    WsjcppLog::info(TAG, "deinit");
    return true;
}

// ---------------------------------------------------------------------

void EmployFlags::setDirectory(const std::string &sDirectory) {
    m_sDirectory = sDirectory;
}

// ---------------------------------------------------------------------

void EmployFlags::clear() {
    std::map<std::string, TableFlagsAttempt *>::iterator it;
    for (it = m_mapFlagAttemps.begin(); it != m_mapFlagAttemps.end(); ++it) {
        delete it->second;
    }
    // TODO removed from folder
}

// ---------------------------------------------------------------------

// add flag attempt
void EmployFlags::insertFlagAttempt(const std::string &sTeamId, const std::string &sFlag) {
    std::map<std::string, TableFlagsAttempt *>::iterator it = m_mapFlagAttemps.find(sTeamId);
    TableFlagsAttempt *pFound = nullptr;
    if (it == m_mapFlagAttemps.end()) { // TODO initialization in different place
        pFound = new TableFlagsAttempt(m_sDirectory + "/attempts/" + sTeamId );
        m_mapFlagAttemps[sTeamId] = pFound;
    } else {
        pFound = it->second;
    }
    pFound->appendAttempt(sTeamId, sFlag);
}

// ---------------------------------------------------------------------

// count of flag attempts for init scoreboard
int EmployFlags::numberOfFlagAttempts(const std::string &sTeamId) {
    std::map<std::string, TableFlagsAttempt *>::iterator it = m_mapFlagAttemps.find(sTeamId);
    TableFlagsAttempt *pFound = nullptr;
    if (it == m_mapFlagAttemps.end()) { // TODO initialization in different place
        pFound = new TableFlagsAttempt(m_sDirectory + "/attempts/" + sTeamId );
        m_mapFlagAttemps[sTeamId] = pFound;
    } else {
        pFound = it->second;
    }
    return pFound->count();
}

// ---------------------------------------------------------------------

// when flag put fail
void EmployFlags::insertFlagPutFail(const Flag &flag, const std::string &sReason) {
    m_pFlagsPutFails->appendFlag(flag, sReason);
}

// ---------------------------------------------------------------------

// when flag check fail
void EmployFlags::insertFlagCheckFail(const Flag &flag, const std::string &sReason) {
    m_pFlagsCheckFails->appendFlag(flag, sReason);
}

// ---------------------------------------------------------------------