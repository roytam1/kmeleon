#pragma once
#include "nsIContentDispatchChooser.h"

class ContentDispatchChooser : public nsIContentDispatchChooser

{
public:
	ContentDispatchChooser(void);
	~ContentDispatchChooser(void);
	
	NS_DECL_ISUPPORTS
	NS_DECL_NSICONTENTDISPATCHCHOOSER
};
