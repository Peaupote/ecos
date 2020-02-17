#define PTS_ACC(J0,J1,J2,J3,N) ((uint64_t*)(\
	 (((uint64_t)(J0))<<39) \
	|(((uint64_t)(J1))<<30) \
	|(((uint64_t)(J2))<<21) \
	|(((uint64_t)(J3))<<12) \
	|((uint64_t)(N<<3))))
