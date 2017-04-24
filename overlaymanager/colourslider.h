#ifndef COLOURSLIDER_H
#define COLOURSLIDER_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QSlider>
#include <QPushButton>

class colourSlider : public QWidget
{
    Q_OBJECT
public:
    explicit colourSlider(QWidget *parent = 0);
    void     setSliderColour(int redSliderValue, int newGreenSliderValue, int newBlueSliderValue);

signals:
    void cancelSliderColourChange();
    void newSliderColourSelected(int, int, int);

public slots:
    void redSliderValueChanged(int newRedSliderValue);
    void greenSliderValueChanged(int newGreenSliderValue);
    void blueSliderValueChanged(int newBlueSliderValue);
    void useNewSliderColour();

private:

    //QFrame *redLine;
    int     redSliderValue;
    int     greenSliderValue;
    int     blueSliderValue;
};

#endif // COLOURSLIDER_H
