#ifndef SERVICE_CHECKER_THREAD_H
#define SERVICE_CHECKER_THREAD_H

#include "config.h"
#include <scoreboard.h>
#include <team.h>
#include <service.h>
#include <employ_flags.h>

// struct 

class ServiceCheckerThread {
	public:
		// enum for checker return code
        static int CHECKER_CODE_UP;
        static int CHECKER_CODE_CORRUPT;
		static int CHECKER_CODE_MUMBLE;
		static int CHECKER_CODE_DOWN;
		static int CHECKER_CODE_SHIT;

		ServiceCheckerThread(Config *pConfig, const Team &teamConf, const Service &serviceConf);
		void start();
		void run();

	private:
		std::string TAG;
		pthread_t m_checkerThread;
		Config *m_pConfig;
		EmployFlags *m_pEmployFlags;
		Team m_teamConf;
		Service m_serviceConf;

		int runChecker(Flag &flag, const std::string &sCommand);
		// int runChecker(Flag &flag, const std::string &sCommand);
		
		// void run();
};

#endif // SERVICE_CHECKER_THREAD_H
