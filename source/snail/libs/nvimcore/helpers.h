/// @headerfile ""

#ifndef SNAIL_LIBS_NVIMCORE_HELPERS_H
#define SNAIL_LIBS_NVIMCORE_HELPERS_H

#include "snail/libs/nvimcore/shellcontents.h"

bool saveShellContents(const ShellContents &s, const QString &filename);
bool isBadMonospace(const QFont &f);

#endif // SNAIL_LIBS_NVIMCORE_HELPERS_H
