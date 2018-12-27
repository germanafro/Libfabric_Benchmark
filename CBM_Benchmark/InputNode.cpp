#include "InputNode.h"
#include "Config.h"

using namespace std;
/* m receive, n transfer node */
InputNode::InputNode(const char *addr, uint64_t flags, Config * config) : Node(addr, flags, config)
{

}

InputNode::~InputNode()
{
	//cleanup 
	
	// note: Endpoints should be cleaned up within the respective class
}


