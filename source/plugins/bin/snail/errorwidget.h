/// @file plugins/bin/snail/errorwidget.h

#ifndef PLUGIN_SNAIL_ERRORWIDGET_H
#define PLUGIN_SNAIL_ERRORWIDGET_H

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

} // namespace::SnailNvimQt

#endif // PLUGIN_SNAIL_ERRORWIDGET_H
