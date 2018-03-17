/// @file plugins/bin/snail/errorwidget.cpp

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "plugins/bin/snail/errorwidget.h"

#define NVIM_ERR_LOGO_HIGHT     64
#define NVIM_ERR_LOGO_WIGHT     64

namespace SnailNvimQt {

ErrorWidget::ErrorWidget(QWidget *parent)
    : QWidget(parent), m_errorLabel(NULL), m_closeButton(NULL)
{
    m_errorLabel = new QLabel();
    m_closeButton = new QPushButton(tr("Retry"));
    m_image = new QLabel();
    m_image->setPixmap(QPixmap(":/error/nvim.png").scaled(NVIM_ERR_LOGO_HIGHT,
                                                          NVIM_ERR_LOGO_WIGHT,
                                                          Qt::KeepAspectRatio));

    connect(m_closeButton, &QPushButton::clicked,
            this, &ErrorWidget::reconnectNeovim);

    QHBoxLayout *inner_layout = new QHBoxLayout();
    inner_layout->addStretch();
    inner_layout->addWidget(m_image);
    inner_layout->addWidget(m_errorLabel);
    inner_layout->addWidget(m_closeButton);
    inner_layout->addStretch();
    QVBoxLayout *outer_layout = new QVBoxLayout();
    outer_layout->addStretch();
    outer_layout->addLayout(inner_layout);
    outer_layout->addStretch();
    setLayout(outer_layout);
}

void ErrorWidget::setText(const QString &text)
{
    m_errorLabel->setText(text);
}

void ErrorWidget::showReconnect(bool on)
{
    m_closeButton->setVisible(on);
}

} // namespace::SnailNvimQt
