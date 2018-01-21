/// @file plugins/bin/snail/winpos_info.cpp

#include "plugins/bin/snail/winpos_info.h"

WinPosInfo::WinPosInfo(QWidget *parent)
    : m_widget(parent)
{
    if(m_widget == NULL)
    {
        m_widget = this;
    }

    setWindowTitle(tr("Geometry"));
    m_xLabel=new QLabel(tr("x():"));
    m_xLabelVal=new QLabel;
    m_yLabel=new QLabel(tr("y():"));
    m_yLabelVal=new QLabel;

    m_FrmLabel = new QLabel(tr("Frame:"));
    m_FrmLabelVal =new QLabel;

    m_posLabel =new QLabel(tr("pos():"));
    m_posLabelVal =new QLabel;

    m_geoLabel =new QLabel(tr("geometry():"));
    m_geoLabelVal =new QLabel;

    m_widthLable =new QLabel(tr("width():"));
    m_widthLableVal =new QLabel;
    m_heightLabel =new QLabel(tr("height():"));
    m_heightLabelVal =new QLabel;

    m_rectLabel =new QLabel(tr("rect():"));
    m_rectLabelVal =new QLabel;

    m_sizeLabel =new QLabel(tr("size():"));
    m_sizeLabelVal =new QLabel;

    m_winLayout =new QGridLayout(this);

    m_winLayout->addWidget(m_xLabel,        0, 0);
    m_winLayout->addWidget(m_xLabelVal,     0, 1);
    m_winLayout->addWidget(m_yLabel,        1, 0);
    m_winLayout->addWidget(m_yLabelVal,     1, 1);
    m_winLayout->addWidget(m_posLabel,      2, 0);
    m_winLayout->addWidget(m_posLabelVal,   2, 1);
    m_winLayout->addWidget(m_FrmLabel,      3, 0);
    m_winLayout->addWidget(m_FrmLabelVal,   3, 1);
    m_winLayout->addWidget(m_geoLabel,      4, 0);
    m_winLayout->addWidget(m_geoLabelVal,   4, 1);
    m_winLayout->addWidget(m_widthLable,    5, 0);
    m_winLayout->addWidget(m_widthLableVal, 5, 1);
    m_winLayout->addWidget(m_heightLabel,   6, 0);
    m_winLayout->addWidget(m_heightLabelVal,6, 1);
    m_winLayout->addWidget(m_rectLabel,     7, 0);
    m_winLayout->addWidget(m_rectLabelVal,  7, 1);
    m_winLayout->addWidget(m_sizeLabel,     8, 0);
    m_winLayout->addWidget(m_sizeLabelVal,  8, 1);

    updateLabel();
}

WinPosInfo::~WinPosInfo()
{
    /* nothing */
}

void WinPosInfo::updateLabel(void)
{
    QString xStr; // x()
    m_xLabelVal->setText(xStr.setNum(m_widget->x()));
    QString yStr; // y()
    m_yLabelVal->setText(yStr.setNum(m_widget->y()));

    QString frameStr; // frameGeometry()
    QString tempStr1, tempStr2, tempStr3, tempStr4;
    frameStr = tempStr1.setNum(m_widget->frameGeometry().x()) + ","
               + tempStr2.setNum(m_widget->frameGeometry().y()) + ","
               + tempStr3.setNum(m_widget->frameGeometry().width()) + ","
               + tempStr4.setNum(m_widget->frameGeometry().height());
    m_FrmLabelVal->setText(frameStr);

    QString positionStr; // pos()
    QString tempStr11, tempStr12;
    positionStr = tempStr11.setNum(m_widget->pos().x()) + ","
                  + tempStr12.setNum(m_widget->pos().y());
    m_posLabelVal->setText(positionStr);

    QString geoStr; // geometry()
    QString tempStr21, tempStr22, tempStr23, tempStr24;
    geoStr = tempStr21.setNum(m_widget->geometry().x()) + ","
             + tempStr22.setNum(m_widget->geometry().y()) + ","
             + tempStr23.setNum(m_widget->geometry().width()) + ","
             + tempStr24.setNum(m_widget->geometry().height());
    m_geoLabelVal->setText(geoStr);

    QString wStr, hStr; // width(), height()
    m_widthLableVal->setText(wStr.setNum(m_widget->width()));
    m_heightLabelVal->setText(hStr.setNum(m_widget->height()));

    QString rectStr; // rect()
    QString tempStr31, tempStr32, tempStr33, tempStr34;
    rectStr = tempStr31.setNum(m_widget->rect().x()) + ","
              + tempStr32.setNum(m_widget->rect().y()) + ","
              // + tempStr33.setNum(/*rect().width()*/width()) + ","
              + tempStr33.setNum(m_widget->width()) + ","
              // + tempStr34.setNum(height()/*rect().height()*/);
              + tempStr34.setNum(m_widget->height());
    m_rectLabelVal->setText(rectStr);

    QString sizeStr;
    QString tempStr41, tempStr42;
    sizeStr = tempStr41.setNum(m_widget->size().width()) + ","
              + tempStr42.setNum(m_widget->size().height());
    m_sizeLabelVal->setText(sizeStr);
}

void WinPosInfo::moveEvent(QMoveEvent *)
{
    updateLabel();
}
void WinPosInfo::resizeEvent(QResizeEvent *)
{
    updateLabel();
}
