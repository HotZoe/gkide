/// @file plugins/bin/snail/shellcontents.h
///
/// A class to hold the contents of the shell / i.e. a grid of characters. This
/// class is meant to hold state about shell contents, but no more - e.g. cursor
/// information should be stored somewhere else.

#ifndef PLUGIN_SNAIL_SHELLCONTENTS_H
#define PLUGIN_SNAIL_SHELLCONTENTS_H

#include "plugins/bin/snail/cell.h"

class ShellContents
{
public:
    ShellContents(int rows, int columns);
    ~ShellContents();
    ShellContents(const ShellContents &other);

    inline int columns(void) const
    {
        return _columns;
    }
    inline int rows(void) const
    {
        return _rows;
    }

    bool fromFile(const QString &path);

    const Cell *data(void);
    Cell &value(int row, int column);
    const Cell &constValue(int row, int column) const;

    int put(const QString &,
            int row, int column,
            QColor fg=Qt::black, QColor bg=Qt::white, QColor sp=QColor(),
            bool bold=false, bool italic=false,
            bool underline=false, bool undercurl=false);

    void clearAll(QColor bg=QColor());
    void clearRow(int r, int startCol=0);
    void clearRegion(int row0, int col0,
                     int row1, int col1, QColor bg=QColor());
    void resize(int rows, int columns);
    void scrollRegion(int row0, int row1,
                      int col0, int col1, int count);
    void scroll(int rows);

private:
    void allocData(void);
    bool verifyRegion(int &row0, int &row1, int &col0, int &col1);

    // row*columns
    Cell *_data;
    static Cell invalidCell;

    int _rows;
    int _columns;

    ShellContents &operator=(const ShellContents &other);
};

#endif // PLUGIN_SNAIL_SHELLCONTENTS_H
