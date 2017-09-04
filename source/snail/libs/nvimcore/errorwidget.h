/// @file snail/libs/nvimcore/errorwidget.h

#ifndef SNAIL_LIBS_NVIMCORE_ERRORWIDGET_H
#define SNAIL_LIBS_NVIMCORE_ERRORWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>

namespace SnailNvimQt {

class ErrorWidget: public QWidget
{
    Q_OBJECT
public:
    ErrorWidget(QWidget *parent=0);
public slots:
    void setText(const QString &text);
    void showReconnect(bool);
signals:
    void reconnectNeovim();

private:
    QLabel *m_errorLabel;
    QLabel *m_image;
    QPushButton *m_closeButton;
};

} // [Namespace] SnailNvimQt

#endif // SNAIL_LIBS_NVIMCORE_ERRORWIDGET_H
