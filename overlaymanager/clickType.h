#ifndef CLICKTYPE_H
#define CLICKTYPE_H

typedef enum
{
    selectClick,         // Perform the option selected
    nextEntry, previousEntry, // Allow buttons to advance/regrade through options
    optionsClick,        // This will generally show (or remove) the menu
    noEvent         // This allows the menu to do highlight when over etc

} click_type_t;

#endif // CLICKTYPE_H
