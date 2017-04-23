#ifndef CSINGLETON_H
#define CSINGLETON_H

// Qt
#include <QtCore/QMutex>

//-------------------------------------------------------------------------------------------------

//! Defines a singleton pattern
template<class T>
class CSingleton
{
public:

    //-------------------------------------------------------------------------------------------------
    // M�thodes de contr�le
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Gets the unique instance of the class
    static T* getInstance()
    {
        if (s_pInstance == NULL)
        {
            s_pInstance = new T();
        }

        return s_pInstance;
    }

    //! Destroys the unique instance of the class
    static void killInstance()
    {
        if (s_pInstance != NULL)
        {
            delete s_pInstance;
        }

        s_pInstance = NULL;
    }

    //-------------------------------------------------------------------------------------------------
    // Protected methods
    //-------------------------------------------------------------------------------------------------

protected:

    //! Constructor
    CSingleton() {}

    //! Destructor
    virtual ~CSingleton() {}

    //-------------------------------------------------------------------------------------------------
    // Properties
    //-------------------------------------------------------------------------------------------------

private:
    static T*		s_pInstance;	// Instance du singleton
    static QMutex	s_mutex;		// Mutex pour acc�s concurrents
};

// Singleton instance
template<class T> T* CSingleton<T>::s_pInstance = NULL;

// Mutex for concurrent access
template<class T> QMutex CSingleton<T>::s_mutex(QMutex::Recursive);

#endif // CSINGLETON_H
