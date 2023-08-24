#ifndef __TutorialApplication_h_
#define __TutorialApplication_h_

#include "BaseApplication.h"

class TutorialApplication : public BaseApplication
{
public:
	TutorialApplication();
	virtual ~TutorialApplication();

protected:
	virtual void createScene();
	using BaseApplication::createCamera;
	using BaseApplication::createViewports;
};

#endif // #ifndef __TutorialApplication_h_