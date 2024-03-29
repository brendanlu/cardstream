from cardstream._wrappers cimport SimEngineBJ

cdef class _SimEngineWrapper: 
    """
    Cython wrapper for C++ simulation interface. 
    """

    cdef SimEngineBJ cppSimEngineBJ
    # cdef SimEngineBJ* cppSimEngineBJ

    def __init__(self, nd, p, d17, nA, strats): 
        """
        Emulates a full game intialization, similar to the constructor in 
        C++ using InitPackage parameter. 

        Will appropriately, and explicitly initialize everything but logging.

        Parameters
        ----------
        nd : int
            The number of decks in the simulation

        p : float
            Deck penetration

        d17 : bool
            Dealer hits on soft 17

        nA : int
            Number of agents in the simulation

        strats : Iterable[Tuple[np.ndarray]]
            >> len(strats) = nA 
            Each item contains a strat tuple in the output format of 
            _strat_to_numpy_arrayfmt

            These represent the agent configurations from the config files
        """
        
        # TODO: validate appropriate params

        self._py_shoe_init(nd, p)
        self._py_set_dealer_17(d17)

        for i in range(nA):
            self._py_set_agent(i, *strats[i])

        return

    def _py_shoe_init(self, nd, p): 
        self.cppSimEngineBJ = SimEngineBJ(nd, p)
        # self.cppSimEngineBJ = new SimEngineBJ(nd, p)

    def _py_set_agent(
        self, 
        agentIdx, 
        char[:,:] hrd,
        char[:,:] sft, 
        char[:,:] splt, 
        double[:] cnt
    ):
        self.cppSimEngineBJ.SetAgent(
            agentIdx, &hrd[0][0], &sft[0][0], &splt[0][0], &cnt[0]
        )

    def _py_set_dealer_17(self, d17): 
        self.cppSimEngineBJ.SetDealer17(d17)
        return

    def py_set_log_file(self, filename):
        self.cppSimEngineBJ.SetLogFile(filename.encode('UTF-8'))
        return

    def py_set_socket_connection(self, ip, port): 
        self.cppSimEngineBJ.SetSocketConnection(ip.encode('UTF-8'), port)
        return

    def py_set_log_level(self, int ll) : 
        self.cppSimEngineBJ.SetLogLevel(ll)
        return

    def run(self, unsigned int nIters): 
        self.cppSimEngineBJ.RunSimulation(nIters)
        return 

    # def __dealloc__(self):
        # del self.cppSimEngineBJ