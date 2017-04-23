// Qt
#include <QApplication>

// Application
#include "freestyleqt.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Launch application
    FreeStyleQt *pApplication = CSingleton<FreeStyleQt>::getInstance();

    // Start
    pApplication->startup();

    // Event loop
    int res = a.exec();

    // Shutdown
    pApplication->shutdown();
    delete pApplication;

    return res;
}
