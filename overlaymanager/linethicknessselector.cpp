#include "linethicknessselector.h"

lineThicknessSelector::lineThicknessSelector(QWidget *parent) : QWidget(parent, Qt::Dialog)
{
    thickness = 2;

    QVBoxLayout *conatiner = new QVBoxLayout();

    QLabel *title = new QLabel(tr("Select line thickness"));
    conatiner->addWidget(title);

    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setMinimum(1);
    slider->setMaximum(64);
    slider->setValue(thickness);
    conatiner->addWidget(slider);

    // Line thickness preview
    horizontalLine = new QFrame();
    horizontalLine->setFrameShape(QFrame::HLine);
    horizontalLine->setFixedHeight(thickness);
    conatiner->addWidget(horizontalLine);

    // Okay/cancel buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *cancelThicknessChangeButton = new QPushButton(tr("Cancel"));
    QPushButton *useNewThicknessButton       = new QPushButton(tr("Okay"));
    buttonLayout->addWidget(cancelThicknessChangeButton);
    buttonLayout->addWidget(useNewThicknessButton);

    conatiner->addItem(buttonLayout);

    // Slots
    connect(slider,SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));
    connect(cancelThicknessChangeButton, SIGNAL(clicked()), this, SIGNAL(cancelThicknessChange()) );
    connect(useNewThicknessButton,       SIGNAL(clicked()), this, SLOT(useNewThickness()) );

    setLayout(conatiner);

    show();

#ifdef Q_OS_ANDROID
    // Manually center on parent as it isn't done automatically. Sigh.
    QPoint topLeft( parent->width()/2  - width()/2,
                    parent->height()/2 - height()/2);

    move(topLeft);

    repaint();
#endif
}

void lineThicknessSelector::setThickness(int newThickness)
{
    thickness = newThickness;
    horizontalLine->setFixedHeight(thickness);
}

// Slots

void lineThicknessSelector::sliderValueChanged(int newThickness)
{
    // Store the latest value
    thickness = newThickness;

    // Update the preview
    horizontalLine->setFixedHeight(thickness);
}

void lineThicknessSelector::useNewThickness()
{
    emit newThicknessSelected(thickness);
}


// Utilities

