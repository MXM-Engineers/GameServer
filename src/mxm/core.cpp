#include "core.h"

GlobalTweakableVariables& GetGlobalTweakableVars()
{
	static GlobalTweakableVariables g;
	return g;
}
