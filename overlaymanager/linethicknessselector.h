#ifndef LINETHICKNESSSELECTOR_H
#define LINETHICKNESSSELECTOR_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QSlider>
#include <QPushButton>

class lineThicknessSelector : public QWidget
{
    Q_OBJECT
public:
    explicit lineThicknessSelector(QWidget *parent = 0);
    void     setThickness(int thickness);

signals:
    void cancelThicknessChange();
    void newThicknessSelected(int);

public slots:
    void sliderValueChanged(int);
    void useNewThickness();

private:

    QFrame *horizontalLine;
    int     thickness;
};

#endif // LINETHICKNESSSELECTORCPP_H
