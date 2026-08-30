#ifndef env_variablesH
#define env_variablesH
#include "field_state_player.h"

namespace Gomoku
{

inline void print_enviropment_variables_hint()
{
	printf("Environment variables\n");
	printf("common_deep (default: %u)\n",State5::common_deep);
	printf("threat_deep  (default: %u)\n",State5::gl_threat_deep);
	printf("prove_mode  (default: %d)\n",(int)State5::prove_mode);
}

inline void scan_enviropment_variables()
{
	const char* sval=getenv("common_deep");
	if(sval!=0&&(*sval)!=0)
		State5::common_deep=atol(sval);

	sval=getenv("threat_deep");
	if(sval!=0&&(*sval)!=0)
		State5::gl_threat_deep=atol(sval);

	sval=getenv("prove_mode");
	if(sval!=0&&(*sval)!=0)
		State5::prove_mode=atol(sval)!=0;

	if(State5::common_deep>State5::gl_threat_deep)
		throw std::runtime_error("error: common_deep>threat_deep");
}

inline void print_used_enviropment_variables(ObjectProgress::log_generator& lg)
{
    lg <<"common_deep="<<State5::common_deep;
    lg <<"threat_deep="<<State5::gl_threat_deep;
    lg <<"prove_mode="<<(int)State5::prove_mode;
}

}

#endif
