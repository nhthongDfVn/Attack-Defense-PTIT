#include <mysql_storage.h>
#include <mysql/mysql.h>
#include <wsjcpp_core.h>

REGISTRY_STORAGE(MySqlStorage)

MySqlStorage::MySqlStorage(int nGameStartUTCInSec, int nGameEndUTCInSec) {
    TAG = "MySqlStorage";
    // m_nGameStartUTCInSec = nGameStartUTCInSec;
    // m_nGameEndUTCInSec = nGameEndUTCInSec;
    m_sGameStartUTCInMS = std::to_string(long(nGameStartUTCInSec)*1000);
    m_sGameEndUTCInMS = std::to_string(long(nGameEndUTCInSec)*1000);
    m_sDatabaseHost = "";
    m_sDatabaseName = "";
    m_sDatabaseUser = "";
    m_sDatabasePass = "";
    m_nDatabasePort = 3306;
}

// ----------------------------------------------------------------------

bool MySqlStorage::applyConfigFromYaml(
    WsjcppYaml &yamlConfig, 
    std::vector<Team> &vTeams, 
    std::vector<Service> &vServicesConf
) {

    m_sDatabaseHost = yamlConfig["mysql_storage"]["dbhost"].getValue();
    WsjcppLog::info(TAG, "mysql_storage.dbhost: " + m_sDatabaseHost);

    m_nDatabasePort = std::atoi(yamlConfig["mysql_storage"]["dbport"].getValue().c_str());
    WsjcppLog::info(TAG, "mysql_storage.dbport: " + std::to_string(m_nDatabasePort));

    m_sDatabaseName = yamlConfig["mysql_storage"]["dbname"].getValue();
    WsjcppLog::info(TAG, "mysql_storage.dbname: " + m_sDatabaseName);


    m_sDatabaseUser = yamlConfig["mysql_storage"]["dbuser"].getValue();
    WsjcppLog::info(TAG, "mysql_storage.dbuser: " + m_sDatabaseUser);

    m_sDatabasePass = yamlConfig["mysql_storage"]["dbpass"].getValue();
    // Log::info(TAG, "mysql_storage.dbpass: " + m_sDatabasePass);

    if (this->m_sDatabaseHost == "") {
        WsjcppLog::err(TAG, "mysql_storage.dbhost - not found");
		return false;
	}

    if (this->m_sDatabaseName == "") {
		WsjcppLog::err(TAG, "mysql_storage.dbname - not found");
		return false;
	}

	if (this->m_sDatabaseUser == "") {
		WsjcppLog::err(TAG, "mysql_storage.dbuser - not found");
		return false;
	}

    if (this->m_sDatabasePass == "") {
		WsjcppLog::err(TAG, "mysql_storage.dbpass - not found");
		return false;
	}

    // try connect to database
    MYSQL *pConn = getDatabaseConnection();
    if (pConn == NULL) {
        WsjcppLog::err(TAG, "Failed to connect.");
        return false;
    }

    return checkAndInstall(pConn);
}

// ----------------------------------------------------------------------

struct MySQLDBUpdate {
    MySQLDBUpdate() : nVersion(0), sQuery("") {};
    MySQLDBUpdate(int nVersion, std::string sQuery) : nVersion(nVersion), sQuery(sQuery) {};
    int nVersion;
    std::string sQuery;
};

// ----------------------------------------------------------------------

bool MySqlStorage::checkAndInstall(MYSQL *pConn) {
    std::string sQuery = "SELECT id, version FROM db_updates ORDER BY id DESC LIMIT 0,1";
    int nCurrVersion = 0;
    if (mysql_query(pConn, sQuery.c_str())) {
        std::string sError(mysql_error(pConn));
        if (sError.find("db_updates' doesn't exist") != std::string::npos) {
            WsjcppLog::info(TAG, "Creating table db_updates .... ");
            std::string sTableDbUpdates = 
                "CREATE TABLE IF NOT EXISTS db_updates ("
			    "  id int(11) NOT NULL AUTO_INCREMENT,"
                "  version INT DEFAULT NULL,"
                "  dt datetime DEFAULT NULL,"
                "  PRIMARY KEY (id)"
                ") ENGINE=InnoDB  DEFAULT CHARSET=utf8 AUTO_INCREMENT=1;";
            if (mysql_query(pConn, sTableDbUpdates.c_str())) {
                std::string sError2(mysql_error(pConn));
                WsjcppLog::err(TAG, "Problem on create table db_updates " + sError2);
                return false;
            } else {
                WsjcppLog::ok(TAG, "Table db_updates success created");
                nCurrVersion = 1;
            }
        } else {
            WsjcppLog::err(TAG, "Problem with database " + sError);
            return false;
        }
    } else {
        MYSQL_RES *pRes = mysql_use_result(pConn);
        MYSQL_ROW row;
        if ((row = mysql_fetch_row(pRes)) != NULL) {
            nCurrVersion = std::stol(std::string(row[1]));
        } else {
            nCurrVersion = 1;
        }
        mysql_free_result(pRes);
    }

    std::vector<MySQLDBUpdate> vUpdates;
    vUpdates.push_back(MySQLDBUpdate(2, // don't change if after commit
        "CREATE TABLE `flags` ("
        "  `id` int UNSIGNED NOT NULL,"
        "  `serviceid` int UNSIGNED DEFAULT NULL,"
        "  `flag_id` varchar(50) DEFAULT NULL,"
        "  `flag` varchar(36) DEFAULT NULL,"
        "  `teamid` varchar(300) DEFAULT NULL,"
        "  `date_start` bigint DEFAULT NULL,"
        "  `date_end` bigint DEFAULT NULL,"
        "  `team_stole` int UNSIGNED DEFAULT NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
    ));

    vUpdates.push_back(MySQLDBUpdate(3,  // don't change if after commit
        "ALTER TABLE `flags` ADD PRIMARY KEY (`id`);"
    ));

    vUpdates.push_back(MySQLDBUpdate(4,  // don't change if after commit
        "ALTER TABLE `flags` ADD INDEX( `teamid`, `team_stole`);"
    ));

    vUpdates.push_back(MySQLDBUpdate(5, // don't change if after commit
        "CREATE TABLE `flags_live` ("
        "  `id` int UNSIGNED NOT NULL,"
        "  `serviceid` int UNSIGNED DEFAULT NULL,"
        "  `flag_id` varchar(50) DEFAULT NULL,"
        "  `flag` varchar(36) DEFAULT NULL,"
        "  `teamid` int UNSIGNED DEFAULT NULL,"
        "  `date_start` bigint DEFAULT NULL,"
        "  `date_end` bigint DEFAULT NULL,"
        "  `team_stole` int UNSIGNED DEFAULT NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
    ));

    vUpdates.push_back(MySQLDBUpdate(6,  // don't change if after commit
        "ALTER TABLE `flags_live` ADD PRIMARY KEY (`id`);"
    ));

    vUpdates.push_back(MySQLDBUpdate(7,  // don't change if after commit
        "ALTER TABLE `flags` MODIFY `id` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=1;"
    ));

    vUpdates.push_back(MySQLDBUpdate(8,  // don't change if after commit
        "ALTER TABLE `flags_live` MODIFY `id` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=1;"
    ));

    // TODO unused
    vUpdates.push_back(MySQLDBUpdate(9, // don't change if after commit
        "CREATE TABLE `flags_attempts` ("
        "  `id` int UNSIGNED NOT NULL,"
        "  `flag` varchar(36) DEFAULT NULL,"
        "  `teamid` int UNSIGNED DEFAULT NULL,"
        "  `dt` bigint DEFAULT NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
    ));

    // TODO unused
    vUpdates.push_back(MySQLDBUpdate(10,  // don't change if after commit
        "ALTER TABLE `flags_attempts` ADD PRIMARY KEY (`id`);"
    ));

    vUpdates.push_back(MySQLDBUpdate(11,  // don't change if after commit
        "ALTER TABLE `flags_live` MODIFY `team_stole` VARCHAR(50) NOT NULL DEFAULT '';"
    ));

    vUpdates.push_back(MySQLDBUpdate(12,  // don't change if after commit
        "ALTER TABLE `flags_live` MODIFY `teamid` VARCHAR(50) NOT NULL DEFAULT '';"
    ));

    vUpdates.push_back(MySQLDBUpdate(13,  // don't change if after commit
        "ALTER TABLE `flags_live` MODIFY `serviceid` VARCHAR(50) NOT NULL DEFAULT '';"
    ));

    vUpdates.push_back(MySQLDBUpdate(14,  // don't change if after commit
        "ALTER TABLE `flags` MODIFY `team_stole` VARCHAR(50) NOT NULL DEFAULT '';"
    ));

    vUpdates.push_back(MySQLDBUpdate(15,  // don't change if after commit
        "ALTER TABLE `flags` MODIFY `teamid` VARCHAR(50) NOT NULL DEFAULT '';"
    ));

    vUpdates.push_back(MySQLDBUpdate(16,  // don't change if after commit
        "ALTER TABLE `flags` MODIFY `serviceid` VARCHAR(50) NOT NULL DEFAULT '';"
    ));

    vUpdates.push_back(MySQLDBUpdate(17,  // don't change if after commit
        "ALTER TABLE `flags_attempts` MODIFY `teamid` VARCHAR(50) NOT NULL DEFAULT '';"
    ));

    vUpdates.push_back(MySQLDBUpdate(18,  // don't change if after commit
        "ALTER TABLE `flags_attempts` MODIFY `id` int(10) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=1;"
    ));

    vUpdates.push_back(MySQLDBUpdate(19, // don't change if after commit
        "CREATE TABLE `flags_put_fails` ("
        "  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "  `serviceid` VARCHAR(50) DEFAULT '',"
        "  `flag_id` varchar(50) DEFAULT NULL,"
        "  `flag` varchar(36) DEFAULT NULL,"
        "  `teamid` VARCHAR(50) DEFAULT '',"
        "  `date_start` bigint DEFAULT NULL,"
        "  `date_end` bigint DEFAULT NULL,"
        "  `team_stole` VARCHAR(50) DEFAULT '',"
        "  `reason` VARCHAR(50) DEFAULT '',"
        "  PRIMARY KEY (id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
    ));

    vUpdates.push_back(MySQLDBUpdate(20, // don't change if after commit
        "CREATE TABLE `flags_check_fails` ("
        "  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "  `serviceid` VARCHAR(50) DEFAULT '',"
        "  `flag_id` varchar(50) DEFAULT NULL,"
        "  `flag` varchar(36) DEFAULT NULL,"
        "  `teamid` VARCHAR(50) DEFAULT '',"
        "  `date_start` bigint DEFAULT NULL,"
        "  `date_end` bigint DEFAULT NULL,"
        "  `team_stole` VARCHAR(50) DEFAULT '',"
        "  `reason` VARCHAR(50) DEFAULT '',"
        "  PRIMARY KEY (id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
    ));

    vUpdates.push_back(MySQLDBUpdate(21, // don't change if after commit
        "CREATE TABLE `services_flags_costs` ("
        "  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "  `serviceid` VARCHAR(50) DEFAULT '',"
        "  `dt` bigint DEFAULT NULL," // date
        "  `flag_cost` int DEFAULT 10,"
        "  PRIMARY KEY (id),"
        "  INDEX(`serviceid`, `dt`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
    ));

    vUpdates.push_back(MySQLDBUpdate(22, // don't change if after commit
        "CREATE TABLE `flags_stolen` ("
        "  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "  `serviceid` VARCHAR(50) DEFAULT '',"
        "  `teamid` VARCHAR(50) DEFAULT '',"
        "  `thief_teamid` VARCHAR(50) DEFAULT '',"
        "  `flag_id` varchar(50) DEFAULT NULL,"
        "  `flag` VARCHAR(50) DEFAULT '',"
        "  `date_start` bigint DEFAULT NULL," // date
        "  `date_end` bigint DEFAULT NULL," // date
        "  `date_action` bigint DEFAULT NULL," // date
        "  `flag_cost` int DEFAULT 0,"
        "  PRIMARY KEY (id),"
        "  INDEX(`serviceid`), "
        "  INDEX(`serviceid`, `thief_teamid`), "
        "  UNIQUE KEY(`serviceid`, `thief_teamid`, `flag_id`, `flag`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
    ));

    vUpdates.push_back(MySQLDBUpdate(23, // don't change if after commit
        "CREATE TABLE `flags_defence` ("
        "  `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "  `serviceid` VARCHAR(50) DEFAULT '',"
        "  `teamid` VARCHAR(50) DEFAULT '',"
        "  `flag_id` varchar(50) DEFAULT NULL,"
        "  `flag` VARCHAR(50) DEFAULT '',"
        "  `date_start` bigint DEFAULT NULL," // date
        "  `date_end` bigint DEFAULT NULL," // date
        "  `flag_cost` int DEFAULT 0,"
        "  PRIMARY KEY (id),"
        "  INDEX(`serviceid`), "
        "  UNIQUE KEY(`serviceid`, `flag_id`, `flag`)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8;"
    ));

    WsjcppLog::info(TAG, "Current database version: " + std::to_string(nCurrVersion));

    bool bFoundUpdate = true;
    while (bFoundUpdate) {
        bFoundUpdate = false;
        MySQLDBUpdate nextUpdate;

        for (unsigned int i = 0; i < vUpdates.size(); i++) {
            if (vUpdates[i].nVersion == nCurrVersion + 1) {
                bFoundUpdate = true;
                nextUpdate = vUpdates[i];
                break;
            }
        }
        if (!bFoundUpdate) {
            break;
        }

        std::string sVersion = std::to_string(nextUpdate.nVersion);
        WsjcppLog::info(TAG, "Install update  " + sVersion + "...");
        if (mysql_query(pConn, nextUpdate.sQuery.c_str())) {
            std::string sError2(mysql_error(pConn));
            WsjcppLog::err(TAG, "Failed install update " + sVersion + ": " + sError2);
            return false;
        } else {
            WsjcppLog::ok(TAG, "Update " + sVersion + " success installed");
            std::string sInsertNewVersion = "INSERT INTO db_updates(version,dt) VALUES(" + sVersion + ",NOW());";
            mysql_query(pConn, sInsertNewVersion.c_str());
            nCurrVersion = nextUpdate.nVersion;
        }
    }

    return true;
}

// ----------------------------------------------------------------------

void MySqlStorage::clean() {
    MYSQL *pConn = getDatabaseConnection();
    // TODO check on null

    {
        std::string sQuery = "DELETE FROM flags_live;";
        if (mysql_query(pConn, sQuery.c_str())) {
            WsjcppLog::err(TAG, "Error insert: " + std::string(mysql_error(pConn)));
            return;
        }
        MYSQL_RES *pRes = mysql_use_result(pConn);
        mysql_free_result(pRes);
    }
    
    {
        std::string sQuery = "DELETE FROM flags;";
        if (mysql_query(pConn, sQuery.c_str())) {
            WsjcppLog::err(TAG, "Error insert: " + std::string(mysql_error(pConn)));
            return;
        }
        MYSQL_RES *pRes = mysql_use_result(pConn);
        mysql_free_result(pRes);
    }
}

// ----------------------------------------------------------------------

MYSQL *MySqlStorage::getDatabaseConnection() {
    std::string sThreadId = WsjcppCore::threadId();
    MYSQL *pDatabase = NULL;
    std::map<std::string,MYSQL *>::iterator it;
    it = m_mapConnections.find(sThreadId);
    if (it == m_mapConnections.end()) {
        pDatabase = mysql_init(NULL);
        if (!mysql_real_connect(pDatabase, 
                this->m_sDatabaseHost.c_str(),
                this->m_sDatabaseUser.c_str(),
                this->m_sDatabasePass.c_str(),
                this->m_sDatabaseName.c_str(), 0, NULL, 0)) {
            WsjcppLog::err(TAG, std::string(mysql_error(pDatabase)));
            WsjcppLog::err(TAG, "Failed to connect.");
            return NULL;
        }
        m_mapConnections[sThreadId] = pDatabase;
    }else{
        pDatabase = it->second;
    }
    return pDatabase; 
}

// ----------------------------------------------------------------------

void MySqlStorage::insertFlagLive(const Flag &flag) {
    MYSQL *pConn = getDatabaseConnection();

    std::string sQuery = "INSERT INTO flags_live(serviceid, flag_id, flag, teamid, "
        "   date_start, date_end, team_stole) VALUES("
        "'" + flag.serviceId() + "', "
        + "'" + flag.id() + "', "
        + "'" + flag.value() + "', "
        + "'" + flag.teamId() + "', "
        + std::to_string(flag.timeStart()) + ", "
        + std::to_string(flag.timeEnd()) + ", "
        + "'" + flag.teamStole() + "'"
        + ");";

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error insert: " + std::string(mysql_error(pConn)));
        return;
    }

    MYSQL_RES *pRes = mysql_use_result(pConn);
    mysql_free_result(pRes);
}

// ----------------------------------------------------------------------

std::vector<Flag> MySqlStorage::listOfLiveFlags() {
    MYSQL *pConn = getDatabaseConnection();

    long nCurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::string sQuery = 
        "SELECT flag_id, serviceid, teamid, flag, date_start, date_end, team_stole "
        "FROM flags_live "
        "WHERE "
        "   date_start > " + m_sGameStartUTCInMS + " "
        "   AND date_end < " + m_sGameEndUTCInMS + " "
        ";";

    std::vector<Flag> vResult;
    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error select (endedFlags): " + std::string(mysql_error(pConn)));
    } else {
        MYSQL_RES *pRes = mysql_use_result(pConn);
        MYSQL_ROW row;
        // output table name
        while ((row = mysql_fetch_row(pRes)) != NULL) {
            Flag flag;
            flag.setId(std::string(row[0]));
            flag.setServiceId(std::string(row[1]));
            flag.setTeamId(std::string(row[2]));
            flag.setValue(std::string(row[3]));
            flag.setTimeStart(std::stol(std::string(row[4])));
            flag.setTimeEnd(std::stol(std::string(row[5])));
            flag.setTeamStole(std::string(row[6]));
            vResult.push_back(flag);
        }
        mysql_free_result(pRes);
    }
    return vResult;
}

// ----------------------------------------------------------------------

std::vector<Flag> MySqlStorage::outdatedFlags(const Team &teamConf, const Service &serviceConf){
    MYSQL *pConn = getDatabaseConnection();

    long nCurrentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    std::string sQuery = 
        "SELECT flag_id, flag, date_start, date_end, team_stole "
        "FROM flags_live "
        "WHERE serviceid = '" + serviceConf.id() + "' "
        "   AND teamid = '" + teamConf.id() + "' "
        "   AND date_end < " + std::to_string(nCurrentTime) + " "
        "   AND date_start > " + m_sGameStartUTCInMS + " "
        "   AND date_end < " + m_sGameEndUTCInMS + " "
        ";";

    std::vector<Flag> vResult;
    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error select (endedFlags): " + std::string(mysql_error(pConn)));
    } else {
        MYSQL_RES *pRes = mysql_use_result(pConn);
        MYSQL_ROW row;
        // output table name
        while ((row = mysql_fetch_row(pRes)) != NULL) {
            Flag flag;
            flag.setServiceId(serviceConf.id());
            flag.setTeamId(teamConf.id());
            flag.setId(std::string(row[0]));
            flag.setValue(std::string(row[1]));
            flag.setTimeStart(std::stol(std::string(row[2])));
            flag.setTimeEnd(std::stol(std::string(row[3])));
            flag.setTeamStole(std::string(row[4]));
            vResult.push_back(flag);
        }
        mysql_free_result(pRes);
    }
    return vResult;
}

// ----------------------------------------------------------------------

void MySqlStorage::updateFlag(const Team &team, const Service &serviceConf, const Flag &sFlag){
    // TODO
}

// ----------------------------------------------------------------------

int MySqlStorage::getDefenceFlags(const std::string &sTeamId, const std::string &sServiceId) {
    int nDefence = 0;
    MYSQL *pConn = getDatabaseConnection();
    std::string sQuery = 
        "SELECT COUNT(*) as defence FROM flags_defence "
        "WHERE serviceid = '" + sServiceId + "' "
        "   AND teamid = '" + sTeamId + "' "
        ";";

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error select (updateScoreboard - calculate defence): " + std::string(mysql_error(pConn)));
    } else {
        MYSQL_RES *pRes = mysql_use_result(pConn);
        MYSQL_ROW row;
        // output table name
        while ((row = mysql_fetch_row(pRes)) != NULL) {
            nDefence += std::stoi(std::string(row[0]));
        }
        mysql_free_result(pRes);
    }
    return nDefence;
}

// ----------------------------------------------------------------------

int MySqlStorage::getDefencePoints(const std::string &sTeamId, const std::string &sServiceId) {
    int nDefence = 0;
    MYSQL *pConn = getDatabaseConnection();
    std::string sQuery = 
        "SELECT SUM(flag_cost) as points FROM flags_defence "
        "WHERE serviceid = '" + sServiceId + "' "
        "   AND teamid = '" + sTeamId + "' "
        ";";

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error select (updateScoreboard - calculate defence): " + std::string(mysql_error(pConn)));
    } else {
        MYSQL_RES *pRes = mysql_use_result(pConn);
        MYSQL_ROW row;
        // output table name
        while ((row = mysql_fetch_row(pRes)) != NULL) {
            if (row[0] != NULL) {
                nDefence += std::stoi(std::string(row[0]));
            }
        }
        mysql_free_result(pRes);
    }
    return nDefence;
}

// ----------------------------------------------------------------------

int MySqlStorage::getStollenFlags(const std::string &sTeamId, const std::string &sServiceId) {
    MYSQL *pConn = getDatabaseConnection();

    int nAttack = 0;
    std::string sQuery = 
        "SELECT COUNT(*) as cnt FROM flags_stolen "
        "   WHERE serviceid = '" + sServiceId + "' "
        "   AND thief_teamid = '" + sTeamId + "' "
        ";";

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error select (updateScoreboard - calculate attack): " + std::string(mysql_error(pConn)));
    } else {
        MYSQL_RES *pRes = mysql_use_result(pConn);
        MYSQL_ROW row;
        // output table name
        while ((row = mysql_fetch_row(pRes)) != NULL) {
            nAttack += std::stoi(std::string(row[0]));
        }
        mysql_free_result(pRes);
    }
    return nAttack;
}

// ----------------------------------------------------------------------

int MySqlStorage::getStollenPoints(const std::string &sTeamId, const std::string &sServiceId) {
    int nDefence = 0;
    MYSQL *pConn = getDatabaseConnection();
    std::string sQuery = 
        "SELECT SUM(flag_cost) as points FROM flags_stolen "
        "WHERE serviceid = '" + sServiceId + "' "
        "   AND thief_teamid = '" + sTeamId + "' "
        ";";

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error select (updateScoreboard - calculate defence): " + std::string(mysql_error(pConn)));
    } else {
        MYSQL_RES *pRes = mysql_use_result(pConn);
        MYSQL_ROW row;
        // output table name
        while ((row = mysql_fetch_row(pRes)) != NULL) {
            if (row[0] != NULL) {
                nDefence += std::stoi(std::string(row[0]));
            }
        }
        mysql_free_result(pRes);
    }
    return nDefence;
}

// ----------------------------------------------------------------------

int MySqlStorage::numberOfFlagSuccessPutted(const std::string &sTeamId, const std::string &sServiceId) {
    MYSQL *pConn = getDatabaseConnection();

    // calculate SLA
    int nFlagsSuccess = 0;
    {
        std::string sQuery = 
            "SELECT SUM(sla) as sla FROM ("
            "   SELECT COUNT(*) as sla FROM flags_live "
            "       WHERE serviceid = '" + sServiceId + "' "
            "           AND teamid = '" + sTeamId + "' "
            "           AND date_start > " + m_sGameStartUTCInMS + " "
            "           AND date_end < " + m_sGameEndUTCInMS + " "
            "   UNION ALL"
            "   SELECT COUNT(*) as sla FROM flags "
            "       WHERE serviceid = '" + sServiceId + "' "
            "           AND teamid = '" + sTeamId + "' "
            "           AND date_start > " + m_sGameStartUTCInMS + " "
            "           AND date_end < " + m_sGameEndUTCInMS + " "
            ") t;";

        // std::cout << sQuery << "\n";
        if (mysql_query(pConn, sQuery.c_str())) {
            WsjcppLog::err(TAG, "Error select (updateScoreboard - calculate SLA): " + std::string(mysql_error(pConn)));
        } else {
            MYSQL_RES *pRes = mysql_use_result(pConn);
            MYSQL_ROW row;
            // output table name
            while ((row = mysql_fetch_row(pRes)) != NULL) {
                nFlagsSuccess += std::stoi(std::string(row[0]));
            }
            mysql_free_result(pRes);
        }
    }

    return nFlagsSuccess;
}

// ----------------------------------------------------------------------

int MySqlStorage::numberOfDefenceFlagForService(const std::string &sServiceId) {
    MYSQL *pConn = getDatabaseConnection();
    int nRet = 0;
    {
        std::string sQuery = 
            "SELECT COUNT(*) as cnt FROM flags_defence WHERE serviceid = '" + sServiceId + "'";

        // std::cout << sQuery << "\n";
        if (mysql_query(pConn, sQuery.c_str())) {
            WsjcppLog::err(TAG, "Error select (updateScoreboard - numberOfDefenceFlagForService): " + std::string(mysql_error(pConn)));
        } else {
            MYSQL_RES *pRes = mysql_use_result(pConn);
            MYSQL_ROW row;
            // output table name
            while ((row = mysql_fetch_row(pRes)) != NULL) {
                nRet += std::stoi(std::string(row[0]));
            }
            mysql_free_result(pRes);
        }
    }
    return nRet;
}

// ----------------------------------------------------------------------

int MySqlStorage::numberOfStolenFlagsForService(const std::string &sServiceId) {
    MYSQL *pConn = getDatabaseConnection();
    
    int nRet = 0;
    {
        std::string sQuery = 
            "SELECT COUNT(*) as cnt FROM flags_stolen WHERE serviceid = '" + sServiceId + "'";

        // std::cout << sQuery << "\n";
        if (mysql_query(pConn, sQuery.c_str())) {
            WsjcppLog::err(TAG, "Error select (updateScoreboard - numberOfDefenceFlagForService): " + std::string(mysql_error(pConn)));
        } else {
            MYSQL_RES *pRes = mysql_use_result(pConn);
            MYSQL_ROW row;
            // output table name
            while ((row = mysql_fetch_row(pRes)) != NULL) {
                nRet += std::stoi(std::string(row[0]));
            }
            mysql_free_result(pRes);
        }
    }
    return nRet;
}

// ----------------------------------------------------------------------

// TODO must deprecated
bool MySqlStorage::findFlagByValue(const std::string &sFlag, Flag &resultFlag) {
    MYSQL *pConn = getDatabaseConnection();
    // TODO check on null

    std::string sQuery = 
        "SELECT teamid, serviceid, flag_id, flag, date_start, date_end, team_stole "
        "FROM flags_live "
        "WHERE "
        "   flag = '" + sFlag + "' "
        "   AND date_start > " + m_sGameStartUTCInMS + " "
        "   AND date_end < " + m_sGameEndUTCInMS + " "
        ";";

    Flag flag;

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error select (findFlagByValue): " + std::string(mysql_error(pConn)));
        return false;
    } else {
        MYSQL_RES *pRes = mysql_use_result(pConn);
        MYSQL_ROW row;
        // output table name
        if ((row = mysql_fetch_row(pRes)) != NULL) {
            resultFlag.setTeamId(std::string(row[0]));
            resultFlag.setId(std::string(row[2]));
            resultFlag.setServiceId(std::string(row[1]));
            resultFlag.setValue(std::string(row[3]));
            resultFlag.setTimeStart(std::stol(std::string(row[4])));
            resultFlag.setTimeEnd(std::stol(std::string(row[5])));
            resultFlag.setTeamStole(std::string(row[6]));
            mysql_free_result(pRes);
            return true;
        }
        mysql_free_result(pRes);
    }
    return false;
}

// ----------------------------------------------------------------------

bool MySqlStorage::updateTeamStole(const std::string &sFlag, const std::string &sTeamId) {
    MYSQL *pConn = getDatabaseConnection();
    // TODO check on null

    std::string sQuery = ""
        "UPDATE flags_live SET team_stole = '" + sTeamId + "' "
        "WHERE flag = '" + sFlag + "'  "
        "  AND team_stole = '' "
        "  AND date_start > " + m_sGameStartUTCInMS + " "
        "  AND date_end < " + m_sGameEndUTCInMS + " "
        ";";


    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error select (updateTeamStole): " + std::string(mysql_error(pConn)));
        return false;
    }

    Flag resultFlag;

    if (!this->findFlagByValue(sFlag, resultFlag)) {
        return false;
    }
    return resultFlag.teamStole() == sTeamId;
}

// ----------------------------------------------------------------------

void MySqlStorage::deleteFlagLive(const Flag &flag) {
    MYSQL *pConn = getDatabaseConnection();
    // TODO check on null

    std::string sQuery = "DELETE FROM flags_live WHERE flag = '" + flag.value() + "';";
    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error delete: " + std::string(mysql_error(pConn)));
        return;
    }
    MYSQL_RES *pRes = mysql_use_result(pConn);
    mysql_free_result(pRes);
    
}

// ----------------------------------------------------------------------

void MySqlStorage::insertToArchive(Flag &flag) {
    MYSQL *pConn = getDatabaseConnection();
    std::string sQuery = "INSERT INTO flags(serviceid, flag_id, flag, teamid, "
        "   date_start, date_end, team_stole) VALUES("
        "'" + flag.serviceId() + "', "
        + "'" + flag.id() + "', "
        + "'" + flag.value() + "', "
        + "'" + flag.teamId() + "', "
        + std::to_string(flag.timeStart()) + ", "
        + std::to_string(flag.timeEnd()) + ", "
        + "'" + flag.teamStole() + "'"
        + ");";

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error insert: " + std::string(mysql_error(pConn)));
        return;
    }

    MYSQL_RES *pRes = mysql_use_result(pConn);
    mysql_free_result(pRes);
}

// ----------------------------------------------------------------------

void MySqlStorage::insertToFlagsDefence(const Flag &flag, int nPoints) {
    MYSQL *pConn = getDatabaseConnection();
    std::string sQuery = "INSERT INTO flags_defence(serviceid, teamid, flag_id, flag, "
        "   date_start, date_end, flag_cost) VALUES("
        "'" + flag.serviceId() + "', "
        + "'" + flag.teamId() + "', "
        + "'" + flag.id() + "', "
        + "'" + flag.value() + "', "
        + std::to_string(flag.timeStart()) + ", "
        + std::to_string(flag.timeEnd()) + ", "
        + std::to_string(nPoints) + " "
        + ");";

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error insert: " + std::string(mysql_error(pConn)));
        return;
    }

    MYSQL_RES *pRes = mysql_use_result(pConn);
    mysql_free_result(pRes);
}

// ----------------------------------------------------------------------

void MySqlStorage::insertToFlagsStolen(const Flag &flag, const std::string &sTeamId, int nPoints) {
    MYSQL *pConn = getDatabaseConnection();
    std::string sQuery = "INSERT INTO flags_stolen(serviceid, teamid, thief_teamid, flag_id, flag,"
        "   date_start, date_end, date_action, flag_cost) VALUES("
        "'" + flag.serviceId() + "', "
        + "'" + flag.teamId() + "', "
        + "'" + sTeamId + "', "
        + "'" + flag.id() + "', "
        + "'" + flag.value() + "', "
        + std::to_string(flag.timeStart()) + ", "
        + std::to_string(flag.timeEnd()) + ", "
        + std::to_string(WsjcppCore::currentTime_milliseconds()) + ", "
        + std::to_string(nPoints) + " "
        + ");";

    if (mysql_query(pConn, sQuery.c_str())) {
        WsjcppLog::err(TAG, "Error insert: " + std::string(mysql_error(pConn)));
        return;
    }

    MYSQL_RES *pRes = mysql_use_result(pConn);
    mysql_free_result(pRes);
}

// ----------------------------------------------------------------------

bool MySqlStorage::isAlreadyStole(const Flag &flag, const std::string &sTeamId) {
    MYSQL *pConn = getDatabaseConnection();
    
    int nRet = 0;
    {
        std::string sQuery = 
            "SELECT COUNT(*) as cnt FROM flags_stolen "
            " WHERE serviceid = '" + flag.serviceId() + "' "
            "   AND thief_teamid = '" + sTeamId + "'"
            "   AND flag_id = '" + flag.id() + "'"
            "   AND flag = '" + flag.value() + "'"
        ;

        // std::cout << sQuery << "\n";
        if (mysql_query(pConn, sQuery.c_str())) {
            WsjcppLog::err(TAG, "Error select (isAlreadyStole): " + std::string(mysql_error(pConn)));
        } else {
            MYSQL_RES *pRes = mysql_use_result(pConn);
            MYSQL_ROW row;
            // output table name
            while ((row = mysql_fetch_row(pRes)) != NULL) {
                nRet += std::stoi(std::string(row[0]));
            }
            mysql_free_result(pRes);
        }
    }
    return nRet > 0;
}

// ----------------------------------------------------------------------

bool MySqlStorage::isSomebodyStole(const Flag &flag) {
    MYSQL *pConn = getDatabaseConnection();
    
    int nRet = 0;
    {
        std::string sQuery = 
            "SELECT COUNT(*) as cnt FROM flags_stolen "
            " WHERE serviceid = '" + flag.serviceId() + "' "
            "   AND teamid = '" + flag.teamId() + "'"
            "   AND flag_id = '" + flag.id() + "'"
            "   AND flag = '" + flag.value() + "'"
        ;

        // std::cout << sQuery << "\n";
        if (mysql_query(pConn, sQuery.c_str())) {
            WsjcppLog::err(TAG, "Error select (isSomebodyStole): " + std::string(mysql_error(pConn)));
        } else {
            MYSQL_RES *pRes = mysql_use_result(pConn);
            MYSQL_ROW row;
            // output table name
            while ((row = mysql_fetch_row(pRes)) != NULL) {
                nRet += std::stoi(std::string(row[0]));
            }
            mysql_free_result(pRes);
        }
    }
    return nRet > 0;
}
