#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include "agent.hpp"
#include "card.hpp"
#include "communication.hpp"
#include "dealer.hpp"
#include "shoe.hpp"
#include "simenginebj.hpp"
#include "strategyinput.hpp"

/*
Nullary constructor, to minimise issues when wrapping with Cython. 
From the Python API end, this will never be called, as the .__init__ method of 
the Python class will always call the one below. 
*/
SimEngineBJ::SimEngineBJ() {}

/*
Initialize with appropriate game shoe. 

Constructor which appropriately seeds the simShoe member object.
This will seed the pseudo random number generator, just this once. 
*/
SimEngineBJ::SimEngineBJ(unsigned int N_DECKS, double penen) : 
    LOGFNAME("LOG.csv"),
    simShoe(N_DECKS, penen),
    nAgents(0)
{}

/*
Even if not used, this serves as a guide for appropriate game initialization. 

Constructor which appropriately constructs all of the simulation objects and 
the game state. 
*/
SimEngineBJ::SimEngineBJ(InitPackage init) :
    LOGFNAME("LOG.csv"),
    simShoe(init.nDecks, init.shoePenentration),
    simDealer(init.dealer17),
    nAgents(init.nAgents)
{
    if (nAgents > MAX_N_AGENTS) {
        nAgents = MAX_N_AGENTS; 
    }

    for (unsigned int i=0; i<nAgents; ++i) {
        agents[i] = Agent(init.strats[i]);
        agents[i].id = i + 1; 
    }
}

/*
Configure the logger, and pass in raw (unmanaged) pointers to member classes. 
The simengine memory manages the logger. Member classes just use a raw pointer.
*/
void SimEngineBJ::InitNewLogging() 
{
    simLog.reset(); 
    simLog = std::make_shared<Logger>();

    Logger *rawPtr = simLog.get(); 

    for (unsigned int i=0; i<nAgents; ++i) {
        agents[i].SetLog(rawPtr); 
    }
}

/*
TODO: Write checks at higher level interface that file format is ok. 
*/
void SimEngineBJ::SetLogFile(std::string filename) 
{
    LOGFNAME = filename; 
}

void SimEngineBJ::SetSocketConnection(std::string ip, int port)
{
    SOCKETIP = ip; 
    SOCKETPORT = port; 
}

/*

*/
void SimEngineBJ::SetLogLevel(int ll) 
{
    LOGLEVEL = ll; 
}

/*
TODO: deprecate this, in preference of initialising everything in constructor.
*/
void SimEngineBJ::SetAgent(
    unsigned int idx, 
    char* hrd, 
    char* sft, 
    char* splt, 
    double* cnt) 
{
    agents[idx] = Agent(hrd, sft, splt, cnt);
    agents[idx].id = idx + 1; 
    nAgents += 1; 
}

/*
Set dealer hit / stand on soft17. Default is false, see Dealer implementation.
*/
void SimEngineBJ::SetDealer17(bool b) 
{
    simDealer.HITSOFT17 = b;
}

/*

*/
void SimEngineBJ::EventFreshShuffle(unsigned int n) 
{
    simShoe.FreshShuffleN(n); 

    for (unsigned int i=0; i<nAgents; ++i) {
        agents[i].FreshShuffleHandler(); 
    }
    
    simLog->FreshShuffleHandler(); 
}

/*

*/
void SimEngineBJ::EventClear() 
{
    // let agents implement wager-settling logic before clearing other objects
    for (unsigned int i=0; i<nAgents; ++i) {
        agents[i].ClearHandler(simDealer);
    }
    
    simShoe.Clear(); 
    simDealer.ClearHandler(); 

    simLog->Clearhandler(); 
}

/*

*/
template<typename targetType> void SimEngineBJ::EventDeal(targetType &target) 
{
    Card dCard = simShoe.Deal();
    target.DealTargetHandler(dCard);

    for (unsigned int i=0; i<nAgents; ++i) {
        agents[i].DealObserveHandler(dCard); 
    }
}

// explicit instantiations
template void SimEngineBJ::EventDeal<Agent>(Agent&); 
template void SimEngineBJ::EventDeal<Dealer>(Dealer&); 

/*

*/
void SimEngineBJ::EventQueryAgent(Agent &targetAgent) 
{
    // passes reference of the simulation dealer state
    if (targetAgent.YieldAction(simDealer) == ACTION::HIT) {
        EventDeal(targetAgent);
        EventQueryAgent(targetAgent); 
        return;
    }
    else /* queryResponse == ACTION::STAND */ {
        return;
    }
}

/*

*/
void SimEngineBJ::EventQueryDealer() 
{
    if (simDealer.YieldAction() == ACTION::HIT) {
        EventDeal(simDealer); 
        EventQueryDealer(); 
        return;
    }
    else /* queryResponse == ACTION::STAND */ {
        return; 
    }
}

/*

*/
void SimEngineBJ::RunSimulation(unsigned long nIters)
{
    auto start = std::chrono::system_clock::now();
    InitNewLogging(); 
    simLog->InitLogFile(LOGFNAME);
    simLog->InitLogSocket(SOCKETIP.c_str(), SOCKETPORT); 
    simLog->SetLogLevel(LOGLEVEL); 

    // check if Logger has valid fhandler object configured
    if (!(*simLog)) {
        simLog->InitLogFile("ERROR.csv"); 
        simLog->WriteRow(
            LOG_LEVEL::BASIC,
            LOG_TYPE::ENGINE, 
            CONTEXT_STRING_1, 
            "ERROR - LOG FILE DESINATION NOT CONFIGURED"
        );
        simLog->ManualFlush(); 
        return; 
    }
    else {
        simLog->WriteRow(
            LOG_LEVEL::BASIC,
            LOG_TYPE::ENGINE, 
            CONTEXT_STRING_1,
            "COMMENCING " 
                + std::to_string(nIters)
                + " SHOE SIMULATIONS"
        );
    }
    
    // do a full shuffle of the shoe
    //
    // directly call the shoe method to avoid affecting log
    // the first iteration of the simulation will reshuffle again anyway
    simShoe.FreshShuffleN(simShoe.N_CARDS); 

    // each iteration is playing one shoe
    for (unsigned long i=0; i<nIters; ++i) {
        // partial fresh shuffle - see Shoe implementation
        EventFreshShuffle(simShoe.N_UNTIL_CUT);

        // simulate until reshuffle condition is met - see Shoe implementation
        //
        // this is currently defined when a certain proportion of the Shoe has 
        // been expended, as per standard table rules
        //
        // each iteration represents 'one hand'
        while (!simShoe.needReshuffle) {
            EventClear(); 
            EventDeal(simDealer); 

            for (unsigned int i=0; i<nAgents; ++i) {
                EventDeal(agents[i]); 
                EventDeal(agents[i]);
            }

            for (unsigned int i=0; i<nAgents; ++i) {
                EventQueryAgent(agents[i]); 
            }

            EventQueryDealer(); 
        }

        simLog->WriteRow(
            LOG_LEVEL::VERBOSE,
            LOG_TYPE::ENGINE, 
            "Shoe Completed", 
            ""
        );
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now() - start
    );

    simLog->WriteRow(
        LOG_LEVEL::BASIC,
        LOG_TYPE::ENGINE, 
        CONTEXT_STRING_1,
        "SUCCESS - SIMULATION COMPLETED IN " 
            + std::to_string(elapsed.count())
            + "ms"
    );

    simLog.reset(); 
    return;
}
