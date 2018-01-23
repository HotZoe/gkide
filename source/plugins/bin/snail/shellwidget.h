/// @file plugins/bin/snail/shellwidget.h

#ifndef PLUGIN_SNAIL_SHELLWIDGET_H
#define PLUGIN_SNAIL_SHELLWIDGET_H

#include <QWidget>
#include "plugins/bin/snail/shellcontents.h"

class ShellWidget: public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor background READ background WRITE setBackground)
    Q_PROPERTY(QColor foreground READ foreground WRITE setForeground)
    Q_PROPERTY(QString fontFamily READ fontFamily)
    Q_PROPERTY(int fontSize READ fontSize)
    Q_PROPERTY(int rows READ rows)
    Q_PROPERTY(int columns READ columns)
    Q_PROPERTY(QSize cellSize READ cellSize)
public:
    ShellWidget(QWidget *parent=0);
    bool setShellFont(const QString &family, int ptSize, int weight = -1,
                      bool italic = false, bool force = false);

    QColor background(void) const;
    QColor foreground(void) const;
    QColor special(void) const;
    QString fontFamily(void) const;
    int fontSize(void) const;
    static ShellWidget *fromFile(const QString &path);

    int rows(void) const;
    int columns(void) const;
    QSize cellSize(void) const;
    const ShellContents &contents(void) const;
    QSize sizeHint(void) const Q_DECL_OVERRIDE;
signals:
    void shellFontChanged(void);
    void fontError(const QString &msg);
public slots:
    void resizeShell(int rows, int columns);
    void setSpecial(const QColor &color);
    void setBackground(const QColor &color);
    void setForeground(const QColor &color);
    void setDefaultFont(void);

    int put(const QString &, int row, int column,
            QColor fg=QColor(), QColor bg=QColor(), QColor sp=QColor(),
            bool bold=false, bool italic=false,
            bool underline=false, bool undercurl=false);

    void clearRow(int row);
    void clearShell(QColor bg);
    void clearRegion(int row0, int col0, int row1, int col1);
    void scrollShell(int rows);
    void scrollShellRegion(int row0, int row1, int col0, int col1, int rows);
    void setLineSpace(unsigned int height);
protected:
    virtual void paintEvent(QPaintEvent *ev) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QResizeEvent *ev) Q_DECL_OVERRIDE;

    void setCellSize(void);
    QRect absoluteShellRect(int row0, int col0, int rowcount, int colcount);

private:
    void setFont(const QFont &);

    ShellContents m_contents;
    QSize m_cellSize;
    int m_ascent;

    QColor m_bgColor;
    QColor m_fgColor;
    QColor m_spColor;

    unsigned int m_lineSpace;
};

#endif // PLUGIN_SNAIL_SHELLWIDGET_H
