#ifndef FREESTYLEQT_H
#define FREESTYLEQT_H

// Application
#include "iservice.h"
#include "csingleton.h"
class Controller;
class BaseView;

class FreeStyleQt : public CSingleton<FreeStyleQt>, public IService
{
public:
    // Constructor
    FreeStyleQt();

    // Startup
    virtual bool startup();

    // Shutdown
    virtual void shutdown();

private:
    // Controller
    Controller *m_pController;

    // View
    BaseView *m_pView;
};

#endif // FREESTYLEQT_H
