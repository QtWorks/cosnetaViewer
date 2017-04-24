#include "colourslider.h"

colourSlider::colourSlider(QWidget *parent) : QWidget(parent, Qt::Dialog)
{
    redSliderValue   = 255;
    greenSliderValue = 0;
    blueSliderValue  = 255;

    QVBoxLayout *container = new QVBoxLayout();

    QLabel *title = new QLabel(tr("Select Colour"));
    container->addWidget(title);

    QSlider *redSlider = new QSlider(Qt::Horizontal);
    redSlider->setMinimum(0);
    redSlider->setMaximum(255);
    redSlider->setValue(redSliderValue);
    container->addWidget(redSlider);

    QSlider *greenSlider = new QSlider(Qt::Horizontal);
    greenSlider->setMinimum(0);
    greenSlider->setMaximum(255);
    greenSlider->setValue(greenSliderValue);
    container->addWidget(greenSlider);

    QSlider *blueSlider = new QSlider(Qt::Horizontal);
    blueSlider->setMinimum(0);
    blueSlider->setMaximum(255);
    blueSlider->setValue(blueSliderValue);
    container->addWidget(blueSlider);

    //Preview - Not needed. Need to merge the values into a single colour not display each one :S
    /*redLine = new QFrame();
    redLine->setFrameShape(QFrame::HLine);
    redLine->setFixedHeight(redSliderValue);
    container->addWidget(redLine);*/

    // Ok/Cancel buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *cancelSliderColourChangeButton = new QPushButton(tr("Cancel"));
    QPushButton *useNewSliderColourButton = new QPushButton(tr("Accept"));
    buttonLayout->addWidget(cancelSliderColourChangeButton);
    buttonLayout->addWidget(useNewSliderColourButton);

    container->addItem(buttonLayout);

    // Slots
    connect(redSlider,                      SIGNAL(valueChanged(int)), this, SLOT(redSliderValueChanged(int)));
    connect(greenSlider,                    SIGNAL(valueChanged(int)), this, SLOT(greenSliderValueChanged(int)));
    connect(blueSlider,                     SIGNAL(valueChanged(int)), this, SLOT(blueSliderValueChanged(int)));
    connect(cancelSliderColourChangeButton, SIGNAL(clicked(bool)),     this, SIGNAL(cancelSliderColourChange()));
    connect(useNewSliderColourButton,       SIGNAL(clicked(bool)),     this, SLOT(useNewSliderColour()));

    setLayout(container);

    show();

#ifdef Q_OS_ANDROID
    // Position of the Widget
    QPoint topLeft( parent->width()/2  - width()/2, parent->height()/2 - height()/2);

    move(topLeft);

    repaint();
#endif
}

void colourSlider::setSliderColour(int newRedSliderValue, int newGreenSliderValue, int newBlueSliderValue)
{
    redSliderValue   = newRedSliderValue;
    greenSliderValue = newGreenSliderValue;
    blueSliderValue  = newBlueSliderValue;
    //redLine->setFixedHeight(redSliderValue);
}

void colourSlider::redSliderValueChanged(int newRedSliderValue)
{
    redSliderValue = newRedSliderValue;
    //redLine->setFixedHeight(redSliderValue);
}

void colourSlider::greenSliderValueChanged(int newGreenSliderValue)
{
    greenSliderValue = newGreenSliderValue;
}

void colourSlider::blueSliderValueChanged(int newBlueSliderValue)
{
    blueSliderValue = newBlueSliderValue;
}

void colourSlider::useNewSliderColour()
{
    emit newSliderColourSelected(redSliderValue, greenSliderValue, blueSliderValue);
}
