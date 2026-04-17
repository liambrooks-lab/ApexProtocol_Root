#include "A_MainframeCoreNode.h"

AAMainframeCoreNode::AAMainframeCoreNode()
{
	NodeFunction = EApexNodeFunction::MainframeCore;
	bRequiresCoreBreachAccess = true;
	NodeIdentifier = TEXT("MAINFRAME_CORE");
}
