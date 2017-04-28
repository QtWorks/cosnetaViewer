#ifndef ROOM_H
#define ROOM_H

// Qt
#include <QObject>

// Application
#include "roommanager_global.h"

class ROOMMANAGERSHARED_EXPORT Room : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int currentMode READ currentMode WRITE setCurrentMode NOTIFY currentModeChanged)
    Q_PROPERTY(int currentDrawingMode READ currentDrawingMode WRITE setCurrentDrawingMode NOTIFY currentDrawingModeChanged)
    Q_ENUMS(AppMode)
    Q_ENUMS(DrawingMode)

public:
    enum Mode {ViewMode=0, PrivateNotesMode, PublicAnnotationMode, PresentationMode, MouseMode};
    enum Action {ToggleModeToolBar=0, Cosneta, ModeOptions, Camera};
    enum DrawingMode {FreeHand=0, Triangle, Square, Circle, Stamp};

    // Only room manager can create/delete room
    friend class RoomManager;

    // Return name
    const QString &name() const;

    // Set name
    void setName(const QString &sName);

    // Return current mode
    int currentMode() const;

    // Set current mode
    void setCurrentMode(int iCurrentMode);

    // Return current drawing mode
    int currentDrawingMode() const;

    // Set current drawing mode
    void setCurrentDrawingMode(int iCurrentDrawingMode);

protected:
    // Constructor
    explicit Room(QObject *parent = 0);

    // Destructor
    ~Room();

private:
    // Room name
    QString m_sName;

    // Room mode
    Mode m_eCurrentMode;

    // Drawing mode
    DrawingMode m_eCurrentDrawingMode;

signals:
    // Name changed
    void nameChanged();

    // Current mode changed
    void currentModeChanged();

    // Current drawing mode changed
    void currentDrawingModeChanged();

public slots:
};

#endif // ROOM_H
