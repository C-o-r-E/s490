#include "JobManager.h"


// Singleton instance
JobManager* JobManager::instance = 0;

JobManager::JobManager() {
}

JobManager* JobManager::getInstance() {
    if (!instance)
        instance = new JobManager;
    return instance;
}

void JobManager::reload(std::deque<JobStruct> jobstruct) {
    // for each job from the new list of jobs
    for (std::deque<JobStruct>::const_iterator it = jobstruct.begin(); it != jobstruct.end(); ++it) {
        // if a job does not exist, create it and return the Job object for it
        Job& job = jobSet[it->id].job;


        if (job.psName != it->psName) {
            // stop old and start new


            // tell the process to quit gracefully
            job.enterGrace();

            // TODO: garbage collect the process and wait until it dies or we have to kill it


        }
    }
}

void JobManager::update(bbtag, const DataPoint&) {

}

void JobManager::eventLoop() {
    // loop forever
    while (true) {
        std::deque<BlackboardConnection*> allCS;
        // decide what might need attention

        for (std::map<bbtag, CSDetails>::iterator it = CSSet.begin(); it != CSSet.end(); ++it) {
            allCS.push_back(&(it->second.controlShell));
        }

        // wait for one (or more) to call for attention
        BlackboardConnection::multiWait(allCS);

        // act based on that
        for (std::deque<BlackboardConnection*>::iterator it = allCS.begin(); it != allCS.end(); ++it) {
            ((ControlShell*) * it)->processMsgQueue();
            ((ControlShell*) * it)->checkForUpdates();
        }
    }
}

void jobManagerUpdateCallback(bbtag _tag, const DataPoint& point) {
    JobManager::getInstance()->update(_tag, point);
}


// debug includes, remove later
#include <iostream>
using namespace std;

void JobManager::__debug__print() {
    for (std::map<bbtag, CSDetails>::const_iterator it = CSSet.begin(); it != CSSet.end(); ++it) {
        cout << "BBTAG\t" << it->first << ":";
        
        for (std::set<jobID>::const_iterator it2 = it->second.depends.begin(); it2 != it->second.depends.end(); ++it2) {
            cout << " " << *it2;
        }
        cout << endl;
    }
    for (std::map<jobID, jobDetails>::const_iterator it = jobSet.begin(); it != jobSet.end(); ++it) {

        cout << "JOBID\t" << it->first << ":";

        for (std::set<bbtag>::const_iterator it2 = it->second.depends.begin(); it2 != it->second.depends.end(); ++it2) {
            cout << " " << *it2;
        }
        cout << endl;

        /*
        cout << it->second.job.pid;
        cout << it->second.job.psName;
        cout << it->second.job.conditions;
        cout << it->second.job.state;
        cout << it->second.job.reverseMapping; //(bbtag->RunCondition*)
        */


    }
}