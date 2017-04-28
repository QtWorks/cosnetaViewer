// Application
#include "modetoolbar.h"
#include "button.h"
#include "room.h"

// Constructor
ModeToolBar::ModeToolBar(QObject *parent) : ToolBar(parent)
{
    QVector<Button *> vButtons;
    vButtons << new Button((int)Room::ViewMode, "", "qrc:/icons/view_mode.svg", Button::SetMode, true, "qrc:/qml/toolbar/qml/submenus/PublicAnnotationWidget.qml", this)
        << new Button((int)Room::PrivateNotesMode, "", "qrc:/icons/personal_notes.svg", Button::SetMode, true, "qrc:/qml/toolbar/qml/submenus/PublicAnnotationWidget.qml", this)
           << new Button((int)Room::PublicAnnotationMode, "", "qrc:/icons/annotate_mode.svg", Button::SetMode, true, "qrc:/qml/toolbar/qml/submenus/PublicAnnotationWidget.qml", this)
                << new Button((int)Room::MouseMode, "", "qrc:/icons/mouse_mode.svg", Button::SetMode, true, "qrc:/qml/toolbar/qml/submenus/MouseModeWidget.qml", this);
    setModel(vButtons);
}
