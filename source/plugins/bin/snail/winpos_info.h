/// @file plugins/bin/snail/winpos_info.h

#ifndef PLUGIN_SNAIL_WINPOS_INFO_H
#define PLUGIN_SNAIL_WINPOS_INFO_H

#include <QDialog>
#include <QLabel>
#include <QGridLayout>

class WinPosInfo : public QDialog
{
    Q_OBJECT

public:
    WinPosInfo(QWidget *parent=NULL);
    ~WinPosInfo();
    void updateLabel(void);

protected:
    void moveEvent(QMoveEvent *);
    void resizeEvent(QResizeEvent *);

private:
    QLabel *m_xLabel;
    QLabel *m_xLabelVal;
    QLabel *m_yLabel;
    QLabel *m_yLabelVal;

    QLabel *m_posLabel;
    QLabel *m_posLabelVal;

    QLabel *m_FrmLabel;
    QLabel *m_FrmLabelVal;

    QLabel *m_geoLabel;
    QLabel *m_geoLabelVal;

    QLabel *m_widthLable;
    QLabel *m_widthLableVal;
    QLabel *m_heightLabel;
    QLabel *m_heightLabelVal;

    QLabel *m_rectLabel;
    QLabel *m_rectLabelVal;
    QLabel *m_sizeLabel;
    QLabel *m_sizeLabelVal;

    QWidget *m_widget;
    QGridLayout *m_winLayout;
};

#endif // PLUGIN_SNAIL_WINPOS_INFO_H
