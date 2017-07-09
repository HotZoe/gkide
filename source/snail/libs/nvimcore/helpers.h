#ifndef QSHELLWIDGET2_UTIL
#define QSHELLWIDGET2_UTIL

#include "snail/libs/nvimcore/shellcontents.h"

bool saveShellContents(const ShellContents& s, const QString& filename);
bool isBadMonospace(const QFont& f);

#endif
