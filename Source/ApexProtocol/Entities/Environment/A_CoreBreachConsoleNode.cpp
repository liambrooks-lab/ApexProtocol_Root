#include "A_CoreBreachConsoleNode.h"

AACoreBreachConsoleNode::AACoreBreachConsoleNode()
{
	NodeFunction = EApexNodeFunction::CoreBreachConsole;
	bRequiresCoreBreachAccess = true;
	NodeIdentifier = TEXT("CORE_BREACH_CONSOLE");
}
