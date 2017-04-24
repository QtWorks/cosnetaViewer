#ifndef DEBUGLOGGER_H
#define DEBUGLOGGER_H

#include <QNetworkAccessManager>

void                   initDebugLogger(void);
void                   closeAndFlushDebugFiler(void);
void                   sendDebugToServerComplete(void);
bool                   debugAvailableToSend(void);
QNetworkAccessManager *sendDebugToServer(void);
void                   init_crash_handler(void);

#endif // DEBUGLOGGER_H
